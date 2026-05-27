# JASP Data Synchronization Architecture
## Reference: `upstream/development` (jasp-stats/jasp-desktop)

---

## 1. Architecture Overview

JASP has a **two-tier persistence model**:

| Layer | Purpose | Technology | Location |
|-------|---------|------------|----------|
| **Internal Database** | Persistent storage for the in-memory `DataSet` | Raw `sqlite3` C API via `DatabaseInterface` | `CommonData/databaseinterface.cpp` |
| **External Data Source** | Original source of imported data | Format-specific importers; Qt SQL for DB | `Desktop/data/importers/` |

Synchronization bridges these layers: data flows from external sources into the internal SQLite store, and re-syncing updates the internal store when the external source changes.

```
+------------------+     +----------------------+
|   QML UI         |     |  DataSetPackage      |
|  (FileMenu,etc)  |---->|  (QAbstractItemModel)|
+------------------+     +----------+-----------+
                                     |
                   +-----------------+------------------+
                   |                                    |
             External Sync                       Internal Model
                   |                                    |
        +---------+-----------+            +-----------+----------+
        | FileEvent/AsyncLoader|            | DatabaseInterface   |
        | (background thread)   |            | (sqlite3 C API)    |
        +---------+-----------+             | internal.sqlite     |
                   |                        +-----------+----------+
        +---------+-----------+                         |
        | Importer (base)      |             +-----------+----------+
        | syncDataSet()        |             | DataSet / Column     |
        | loadDataSet()        |             | Filter / Label       |
        +---------+-----------+             +----------------------+
                   |
        +---------+-----------+
        | Concrete Importers   |
        | CSV/ReadStat/Excel/  |
        | ODS/RData/JASP/DB    |
        +----------------------+

+===================================================================+
|                    R PROCESS (ENGINE)                              |
|  +---------+     +---------+     +-----------------------+         |
|  |RBridge  |<--->|DataBridge|<--->|DatabaseInterface     |         |
|  |(rbridge)|     |         |     |(separate connection)  |         |
|  +---------+     +---------+     +-----------------------+         |
+===================================================================+
```

---

## 2. Internal Database: `DatabaseInterface`

**File:** `CommonData/databaseinterface.cpp` (~2585 lines)

### 2.1 Singleton

- `DatabaseInterface::singleton()` -- shared across Desktop and Engine processes.
- Uses raw **sqlite3 C API** (not Qt SQL) for maximum control.

### 2.2 Schema (`internalDbDefinition.h`)

| Table | Purpose | Key Columns |
|-------|---------|-------------|
| `DataSets` | One row per open dataset | `id`, `dataFilePath`, `databaseJson`, `revision`, `dataFileSynch` |
| `Columns` | One row per column | `id`, `dataSet`, `name`, `columnType`, `rCode`, `revision` |
| `Filters` | One row per filter | `id`, `dataSet`, `rFilter`, `revision` |
| `Labels` | One row per label value | `id`, `columnId`, `value`, `label` |
| `DataSet_<id>` | Dynamic data tables | `rowNumber`, `Filter_<id>`, `Column_<colId>` |

### 2.3 Revision-Based Change Detection

- `DataSets`, `Columns`, `Filters` have a `revision` field.
- Every mutation increments the revision.
- Engines periodically check `checkForUpdates()` -- compares local vs DB revision to detect what changed.
- This is how the R Engine and Desktop stay in sync without full data transfers.

### 2.4 Transaction Management

- `transactionWriteBegin/End`: Uses `BEGIN EXCLUSIVE` (blocks other writers).
- `transactionReadBegin/End`: Uses `BEGIN DEFERRED` (shared read lock).
- Both handle `SQLITE_BUSY` with sleep+retry up to 90 seconds.
- Thread-local nesting counters prevent deadlock.

### 2.5 Thread Safety -- Per-Thread SQLite Connections

```cpp
std::map<std::thread::id, sqlite3*> _dbs;
```

- Each thread gets its **own sqlite3 connection** to the same `.sqlite` file.
- Protected by `_dbCheckMutex` (std::mutex).
- Opened with `SQLITE_OPEN_NOMUTEX` -- JASP handles all serialization.
- Applications must call `preloadInterfaceForThread()` from worker threads.

### 2.6 Batched Operations

- `dataSetBatchedValuesUpdate()`: Parameterized INSERT/REPLACE in a single transaction.
- `dataSetBatchedValuesLoad()`: Concurrent multi-thread loading, splitting columns by `hardware_concurrency()`.
- `_doubleTroubleBinder/Reader()`: Handles NaN/Inf as strings since SQLite lacks native support.

### 2.7 WAL Checkpoints

- A 5-minute timer runs `sqlite3_wal_checkpoint` to prevent the Write-Ahead Log from growing unbounded.
- Checkpointed during `beginSynchingData()` and `endSynchingData()`.

### 2.8 Schema Evolution

- `upgradeDBFromVersion()` handles migration when loading old `.jasp` files.
- Uses `ALTER TABLE ... ADD/DROP COLUMN` inside a write transaction.
- Tracks per-version schema changes (e.g., `"description"` added in 0.18.2, `"emptyValuesJson"` in 0.19.0).

---

## 3. Data File Synchronization

### 3.1 Key Classes

| Class | File | Role |
|-------|------|------|
| `AsyncLoader` | `Desktop/data/asyncloader.cpp` | Background I/O orchestrator; lives in a `QThread` |
| `DataSetLoader` | `Desktop/data/datasetloader.cpp` | Static helper -- selects importer by file extension |
| `Importer` (base) | `Desktop/data/importers/importer.cpp` | Abstract base with `loadDataSet()` / `syncDataSet()` |
| `ImportDataSet` | `Desktop/data/importers/importdataset.h` | Intermediate data structure (vector of `ImportColumn*`) |
| `DataSetPackage` | `Desktop/data/datasetpackage.cpp` | Central singleton, QAbstractItemModel, owns DataSet |
| `FileEvent` | `Desktop/data/fileevent.cpp` | Request/response envelope for I/O operations |
| `DataSet` | `CommonData/dataset.h` | In-memory data container, owns Columns + Filter |
| `InitColumnTask` | (in `importer.cpp`) | QRunnable for parallel column initialization via QThreadPool |

### 3.2 Full Load Flow

```
User opens file
    |
    v
FileMenu::open() -> creates FileEvent(FileOpen)
    |
    v
MainWindow::dataSetIORequestHandler() -> connects FileEvent::completed
    -> _loader->io(event)                [AsyncLoader on main thread]
    |
    v
AsyncLoader::io() -> emit beginLoad(event)  [Qt::QueuedConnection to
                                              AsyncLoaderThread]
    |
    v
AsyncLoader::loadTask() -> loadPackage()
    |
    v
DataSetLoader::loadPackage(path, extension)
    -> getImporter() returns CSVImporter (e.g.)
    -> importer->loadDataSet(locator, progress)
    |
    v
Importer::loadDataSet():
    1. pkg->beginLoadingData()
       - enginesPrepareForData() (pause R engines)
       - doWalCheckPoint() (flush SQLite WAL)
       - beginResetModel() (Qt model reset)
    2. loadFile() -> virtual method, returns ImportDataSet
       - CSVImporter: tokenizer with delimiter detection
       - ReadStatImporter: wraps readstat C library for SPSS/SAS/Stata
       - ExcelImporter, ODSImporter, etc.
    3. dataSet->beginBatchedToDB()
    4. dataSet->setColumnCount/RowCount()
    5. For each column: QThreadPool::start(InitColumnTask)
       - InitColumnTask::run() -> column->initFromLookups()
       - Each writes to internal SQLite via DatabaseInterface
    6. Wait for all columns -> dataSet->endBatchedToDB()
    7. pkg->endLoadingData()
       - doWalCheckPoint()
       - endResetModel()
       - enginesReceiveNewData() (send data to R engines)
       - emit modelInit()
    |
    v
FileEvent::setComplete() -> emit completed
    |
    v
MainWindow::dataSetIOCompleted()
    -> setCurrentFile(), check timestamps, populate UI
```

### 3.3 Synchronization Sync Flow

```
User clicks "Sync Data" or timer fires
    |
    v
FileEvent(FileSyncData) -> AsyncLoader -> DataSetLoader::syncPackage()
    |
    v
Importer::syncDataSet(locator, dataSet, progress):
    |
    1. Load fresh data from external source (loadFile())
    2. Collect old non-computed columns -> oldColumns set
    3. For each import column matching an existing column by name:
       - isColumnDifferentFromStringLookUps() -> if data differs, mark
         as changed
       - If same but rowCount changed -> mark as changed (row count
         update)
    4. For unmatched import columns -> new columns
    5. Old columns not matched -> orphaned (will be deleted)
    6. Replace matching: orphaned columns overwritten by new columns
    7. Run all InitColumnTask in parallel via QThreadPool
    8. Wait for all tasks (busy-poll with 10us sleep between checks)
    9. Delete truly orphaned columns
    10. Emit datasetChanged(changedColumns, missingColumns,
                            changeNameColumns, rowCountChanged,
                            hasNewColumns)
    |
    v
DataSetPackage::endSynchingData()
    -> endLoadingData() (reset model, notify engines)
    -> emit datasetChanged() [Qt signal -> UI]
```

### 3.4 External File Watching (`QFileSystemWatcher`)

After loading a non-JASP file, `AsyncLoader::loadPackage()` calls:
```cpp
pkg->setSynchingExternally(true);
```

`FileMenu` uses `QFileSystemWatcher` to monitor the data file path:
- `_watcher.addPath(path)` when `setCurrentDataFile()` is called.
- **Note:** Automatic `fileChanged` signal handling is **currently disabled** via `#ifdef NOT_IGNORING_SYNCHING` guards throughout the codebase.

### 3.5 Timestamp-Based Detection on JASP File Load

When opening a `.jasp` file that references an external data file:
```cpp
if (currentTimestamp > _package->dataSet()->dataFileTimestamp())
    setCheckAutomaticSync(true);
    // Prompts: "The datafile that was used by this JASP file was
    //           modified. Do you want to reload the analyses with
    //           this new data?"
```

The `checkDoSync()` method uses `Qt::BlockingQueuedConnection` -- the importer thread blocks until the user responds to the dialog.

### 3.6 Manual Edits Disable Sync

When the user edits data in the spreadsheet, `setManualEdits(true)` automatically disables external sync:
```cpp
if(_manualEdits)
    setSynchingExternally(false);  // Turn off external sync
```
This ensures user edits are never overwritten by external changes.

### 3.7 Relink Data

- User clicks "Sync Data" in File Menu.
- If a database connection is configured, creates a `FileSyncData` event with DB info.
- Otherwise opens a file browser with the current data file path pre-selected -- user can browse to a **new location**.
- `setSynchingExternallyFriendly()` shows a dialog: *"Generate Data File"* / *"Reload Data File"* / *"Find Data File"*.

---

## 4. Database Synchronization

### 4.1 Supported External Database Types

Defined in `Common/utilenums.h`:
```cpp
DECLARE_ENUM(DbType, NOTCHOSEN, QDB2, QMYSQL, QOCI, QODBC,
             QPSQL, QSQLITE);
```

These map to Qt SQL driver names used by `QSqlDatabase::addDatabase()`.

### 4.2 `DatabaseConnectionInfo` -- Connection Credentials

**File:** `CommonData/databaseconnectioninfo.cpp`

| Field | Type | Purpose |
|-------|------|---------|
| `_dbType` | `DbType` | Database type |
| `_hostname` | `QString` | Server hostname |
| `_port` | `int` | Server port |
| `_database` | `QString` | Database name (file path for SQLite) |
| `_username` | `QString` | Login username |
| `_password` | `QString` | Login password |
| `_query` | `QString` | SQL SELECT query to run |
| `_interval` | `int` | Sync interval in minutes (0 = no periodic sync) |
| `_rememberMe` | `bool` | Persist password in `.jasp` files |

Key methods:
- **`connect()`**: Opens a Qt SQL connection (`QSqlDatabase::addDatabase` → `db.open()`).
- **`runQuery()`**: Executes the stored SQL query, validates it's SELECT, returns `QSqlQuery`.
- **`startSynching()`**: Sets up a `QTimer` (`_syncher`) that fires every N minutes. Emits `askPassword()` signal if password not stored.
- **`toJson()` / `fromJson()`**: Serializes to/from JSON for persistence in `.jasp` files.

### 4.3 `DatabaseImporter` -- Import Pipeline

**File:** `Desktop/data/importers/databaseimporter.cpp`

`loadFile()` method:
1. Parses locator as JSON → creates `DatabaseConnectionInfo`
2. Connects to external DB via `_info->connect()`
3. Runs query via `_info->runQuery()` → `QSqlQuery`
4. Creates `ImportDataSet` with `DatabaseImportColumn` objects per result column
5. Iterates all rows, pushing values via `addValue()`
6. Closes connection, returns `ImportDataSet`

### 4.4 Periodic Database Re-Sync

```
QTimer fires (every N minutes)
    |
    v
DataSetPackage::_databaseIntervalSyncher
    -> synchingIntervalPassed signal
    |
    v
Creates FileEvent(FileSyncData) with database info
    -> AsyncLoader -> Importer::syncDataSet()
    |
    v
Same column-by-column comparison as file sync
    -> changed columns, new columns, missing columns
    -> InitColumnTask parallel update
    -> emit datasetChanged()
    |
    v
UI updates only affected columns
```

### 4.5 `DatabaseFileMenu` -- QML UI Binding

**File:** `Desktop/widgets/filemenu/databasefilemenu.h`

Exposed to QML via `Q_PROPERTY`:
- `connect()` -- calls `_info.connect()`
- `runQuery()` -- executes query, stores results in `_queryResult`
- `importResults()` -- triggers import into JASP
- `browseDbFile()` -- file picker for SQLite databases

---

## 5. Engine-Side Data Access

### 5.1 `DataBridge` -- Engine Interface

**File:** `CommonData/databridge.cpp`

- Holds a pointer to `DatabaseInterface` singleton.
- `provideAndUpdateDataSet()` lazily creates/updates a `DataSet` and checks revisions.
- Exposes column CRUD methods called from R via the R Bridge.

### 5.2 `R Bridge` -- C++ ↔ R Communication

**File:** `CommonData/rbridge.cpp` (~962 lines)

`extern "C"` functions callable from the R process:
- `rbridge_readDataSet()` / `rbridge_readFullDataSet()` / `rbridge_readDataSetRequested()` -- read columns into `RBridgeColumn` arrays (with labels, types, filters).
- `rbridge_createColumn()` / `rbridge_deleteColumn()` / `rbridge_setColumnDataAndType()` -- computed column management.
- `rbridge_applyFilter()` -- evaluates R filter expressions.
- `rbridge_runModuleCall()` -- launches analyses.
- `rbridge_evalRComputedColumn()` -- evaluates R code for computed columns.

All data transfer uses `ColumnEncoder` for name encoding (to handle special characters in R).

---

## 6. Thread Safety & Concurrency

| Mechanism | Location | Purpose |
|-----------|----------|---------|
| `Qt::QueuedConnection` for all cross-thread signals | `AsyncLoader`, `DataSetPackage` | Thread-safe event delivery |
| `Qt::BlockingQueuedConnection` | `Importer::syncDataSet()` | Block importer thread until user responds to dialog |
| `_dbCheckMutex` (std::mutex) | `DatabaseInterface` | Protects per-thread connection registry |
| `_loadMutex` (std::mutex) | `DatabaseInterface` | Serializes database loading across threads |
| Per-thread `sqlite3*` connections | `DatabaseInterface::_dbs` | Each thread has its own SQLite connection handle |
| `thread_local` transaction depth counters | `databaseinterface.cpp` | Prevents nested transaction deadlocks |
| `SQLITE_OPEN_NOMUTEX` | `sqlite3_open_v2()` flags | Disables SQLite internal mutexes (JASP handles it) |
| `_serialFinishing` (QMutex) | `Importer` | Serializes import column finish callbacks from QThreadPool |
| `_synchingData` flag | `DataSetPackage` | Prevents `setManualEdits(true)` during sync operations |
| `asyncLoader` runs in dedicated QThread | `AsyncLoaderThread` | All file I/O off the main thread |
| Engines are separate processes | `EngineRepresentation` | R analyses run in isolated OS processes |
| `beginLoadingData/endLoadingData` bracketing | `DataSetPackage` | Coordinates engine pause, WAL checkpoint, model reset |

---

## 7. Summary: Key Architectural Patterns

1. **Thread-isolated I/O**: All file/DB I/O runs in `AsyncLoaderThread`, a dedicated QThread. The main thread only handles UI/model updates.

2. **Per-thread SQLite connections**: No shared sqlite3 handles; each thread gets its own connection to the same file. Mutexes protect the connection registry and serialize writes.

3. **Revision-based incremental sync**: The internal DB uses revision counters on DataSets, Columns, and Filters. The Engine checks these revisions rather than re-reading all data.

4. **Column-level diffing during sync**: `syncDataSet()` compares columns by name, checks data differences via `isColumnDifferentFromStringLookUps()`, and only updates what changed. Uses QThreadPool for parallel column initialization.

5. **Manual edits disable external sync**: `setManualEdits(true)` → `setSynchingExternally(false)` ensures user edits are never silently overwritten.

6. **Blocking user dialog during sync**: The importer thread blocks with `BlockingQueuedConnection` while waiting for the user to confirm "do you want to reload?".

7. **Engine isolation**: R analyses run in separate OS processes. Communication happens via the R Bridge (`extern "C"` functions) and the internal SQLite database (shared file, per-process connection).
