# JASP Desktop — Agent Guide

## Build

```bash
cmake -GNinja -S . -B build -DBUILD_TESTS=ON
cmake --build build --target CommonData    # library target only
cmake --build build                        # everything (slow)
cmake --build build --target JASP          # desktop app only
```

- Use the existing `build/` directory — it is already configured.
- Re-run `cmake build/` after adding new `.cpp`/`.h` files (CMake uses `GLOB_RECURSE` in `CommonData/CMakeLists.txt`).
- `librt` is auto-detected and linked by `Tools/CMake/Libraries.cmake`.
- Engine binary lands in `build/Desktop/` alongside the `JASP` executable.

## Tests

All test targets depend on `JASPDesktopLib` (cannot build independently).

```bash
cmake --build build --target JASPTest
xvfb-run build/Tests/JASPTest                           # run all (needs xvfb)
xvfb-run build/Tests/JASPTest testSyncerStartStopFileSyncing  # single test by name
ctest -R testDataImport --output-on-failure             # or via ctest
```

Test names (use `-functions` on binary to list all):
- `JASPTest` — data import + syncer tests
- `JASPTestEngine`, `JASPTestDebugData`, `JASPTestCsvPrev`, `JASPQuickTest`

Run with `QT_QPA_PLATFORM=offscreen` instead of `xvfb-run` if you don't want a virtual display. The test library is at `Tests/TestLibrary/`.

## Library architecture (dependency order)

```
Common → CommonData → QMLComponents → JASPEngine / JASPDesktopLib → JASP
```

| Library | Type | Description |
|---|---|---|
| `Common` | static | Shared utilities, `ColumnEncoder`, logging, JSON |
| `CommonData` | static | Data layer: `DataSet`, `Workspace`, `Column`, `Filter`, SQLite, R bridge |
| `QMLComponents` | static QML module | QML controls, URI `JASP.Controls` |
| `Desktop/JASPDesktopLib` | static | Desktop UI logic |
| `Engine/JASPEngine` | exec | R engine process |
| `SyntaxInterface` | shared | jaspSyntax R bridge lib |

## Key classes

- `DataSet` — owns columns, filters, `ColumnEncoder`, `DataSetSyncer`
- `Workspace` — owns map of datasets, manages shown/filter state
- `DataSetSyncer` — per-dataset file watcher + DB interval syncer (created in `DataSet` ctor)
- `ColumnEncoder` — per-dataset column name encoding singleton with context pointer (`ColumnEncoder::setCurrentEncoder()`)
- `DatabaseConnectionInfo` — DB interval polling timer (owned by `DataSetSyncer`)
- `DataSetPackage` — singleton desktop wrapper around Workspace

## Git notes

- No enforced prefix convention (`feature/`, `bugfix/` not used).
- Bot branches prefixed `bot` (e.g., `botDataSetSynch`, `botDev`).
- Upstream branch: `origin/development`. Forks: `joris/development`, `bruno/development`.

## Qt quirks

- Tests use `QApplication` (Widgets-based), need a display. Use `xvfb-run`.
- `#ifdef NOT_IGNORING_SYNCHING` — never defined anywhere, dead code.
- `FileEvent::FileSyncData` — was dead/never existed, now added to enum.
- `DataSet::setDataFileAndTimeStamp` (overload) exists alongside `setDataFile` (single string).

## Conventions

- `#include` paths: Desktop headers use `data/datasetpackage.h`, CommonData/Common headers use flat `"dataset.h"`.
- When adding tests, add slots to `Tests/testall.h` and implementations to `Tests/testall.cpp` (no standalone test target needed).
- Always add `friend class DataSet;` if constructing `ColumnEncoder` directly.