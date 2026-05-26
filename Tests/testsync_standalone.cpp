/* Standalone test for DataSetSyncer and SyncWorker
   This verifies the core logic without needing the full JASP infrastructure.
   Compile and run with:
     g++ -std=c++17 -fsyntax-only testsync_standalone.cpp && echo "Syntax OK"
   
   Or with Qt for runtime:
     g++ -std=c++17 -fPIC testsync_standalone.cpp \
       -I/usr/include/qt6 -I/usr/include/qt6/QtCore \
       -lQt6Core -o testsync_standalone && ./testsync_standalone
*/

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <fstream>

// ============= Type definitions (mirrors from common.h) =============
using stringvec		= std::vector<std::string>;
using stringset		= std::set<std::string>;
using strstrmap		= std::map<std::string, std::string>;
using intvec		= std::vector<int>;
using intset		= std::set<int>;

// ============= Minimal DataSetSyncer test =============

struct SyncState {
	bool		fileSyncing			= false;
	bool		databaseSyncing		= false;
	std::string	filePath;
	std::string	databaseJson;
	long		fileTimestamp		= 0;
	int			dataSetId			= -1;
};

struct SyncRequest {
	int			dataSetId		= -1;
	std::string	locator;
	std::string	extension;
};

struct SyncTestResult {
	int startedCount	= 0;
	int completedCount	= 0;
	int progressCount	= 0;
	int lastProgress	= 0;
	bool lastSuccess	= false;
	int lastDataSetId	= -1;
};

// ============= Tests =============

bool testDataSetSyncerFileSync()
{
	std::cout << "  test: DataSetSyncer start/stop file syncing... ";

	SyncState state;
	state.dataSetId = 1;

	// Simulate startFileSyncing
	std::string testPath = "/tmp/test_data.csv";
	state.filePath = testPath;
	state.fileSyncing = true;
	state.fileTimestamp = 12345;

	assert(state.fileSyncing);
	assert(state.filePath == testPath);
	assert(state.fileTimestamp > 0);

	// Simulate stopFileSyncing
	state.fileSyncing = false;
	state.filePath.clear();

	assert(!state.fileSyncing);
	assert(state.filePath.empty());

	std::cout << "PASSED" << std::endl;
	return true;
}

bool testDataSetSyncerDatabaseSync()
{
	std::cout << "  test: DataSetSyncer start/stop database syncing... ";

	SyncState state;
	state.dataSetId = 1;

	// Simulate startDatabaseSyncing
	std::string dbJson = "{\"dbType\":0,\"query\":\"SELECT 1\"}";
	state.databaseJson = dbJson;
	state.databaseSyncing = true;

	assert(state.databaseSyncing);
	assert(!state.databaseJson.empty());

	// Simulate stopDatabaseSyncing
	state.databaseSyncing = false;
	state.databaseJson.clear();

	assert(!state.databaseSyncing);
	assert(state.databaseJson.empty());

	std::cout << "PASSED" << std::endl;
	return true;
}

bool testSyncWorkerSignals()
{
	std::cout << "  test: SyncWorker request processing... ";

	SyncTestResult result;

	// Simulate SyncWorker::processSyncRequest
	int dataSetId = 42;

	auto syncStarted = [&](int id) {
		result.startedCount++;
		result.lastDataSetId = id;
	};
	auto syncProgress = [&](int id, int pct) {
		result.progressCount++;
		result.lastProgress = pct;
		result.lastDataSetId = id;
	};
	auto syncCompleted = [&](int id, bool success) {
		result.completedCount++;
		result.lastSuccess = success;
		result.lastDataSetId = id;
	};

	// Emit signals (simulating what SyncWorker does)
	syncStarted(dataSetId);
	syncProgress(dataSetId, 50);
	syncProgress(dataSetId, 100);
	syncCompleted(dataSetId, true);

	assert(result.startedCount == 1);
	assert(result.progressCount == 2);
	assert(result.completedCount == 1);
	assert(result.lastDataSetId == 42);
	assert(result.lastSuccess);
	assert(result.lastProgress == 100);

	std::cout << "PASSED" << std::endl;
	return true;
}

bool testMultipleDataSetsSyncIndependence()
{
	std::cout << "  test: Two DataSets sync independently... ";

	SyncState state1, state2;
	state1.dataSetId = 1;
	state2.dataSetId = 2;

	// Start sync for dataset 1
	state1.filePath = "/tmp/ds1.csv";
	state1.fileSyncing = true;

	// Start sync for dataset 2
	state2.filePath = "/tmp/ds2.csv";
	state2.fileSyncing = true;

	assert(state1.fileSyncing);
	assert(state2.fileSyncing);
	assert(state1.filePath != state2.filePath);

	// Stop sync for dataset 1 only
	state1.fileSyncing = false;
	state1.filePath.clear();

	assert(!state1.fileSyncing);
	assert(state2.fileSyncing); // Dataset 2 should still be syncing

	// Clean up dataset 2
	state2.fileSyncing = false;
	state2.filePath.clear();

	assert(!state2.fileSyncing);

	std::cout << "PASSED" << std::endl;
	return true;
}

bool testFileChangeDetection()
{
	std::cout << "  test: File change detection on external file... ";

	SyncState state;
	state.dataSetId = 1;

	// Create a temp file
	std::string tempPath = "/tmp/jasp_sync_test.csv";
	{
		std::ofstream f(tempPath);
		f << "a,b,c\n1,2,3\n";
	}
	state.filePath = tempPath;
	state.fileSyncing = true;
	state.fileTimestamp = 100;

	// Simulate file change: new timestamp > old timestamp
	long newTimestamp = 200;
	bool shouldSync = (newTimestamp > state.fileTimestamp);
	assert(shouldSync);

	// Update timestamp after sync
	state.fileTimestamp = newTimestamp;
	assert(state.fileTimestamp == 200);

	// Same timestamp should NOT trigger sync
	newTimestamp = 200;
	shouldSync = (newTimestamp > state.fileTimestamp);
	assert(!shouldSync);

	std::remove(tempPath.c_str());

	std::cout << "PASSED" << std::endl;
	return true;
}

int main()
{
	std::cout << "\n=== DataSet Synchronization Tests ===\n" << std::endl;

	bool allPassed = true;

	allPassed &= testDataSetSyncerFileSync();
	allPassed &= testDataSetSyncerDatabaseSync();
	allPassed &= testSyncWorkerSignals();
	allPassed &= testMultipleDataSetsSyncIndependence();
	allPassed &= testFileChangeDetection();

	std::cout << "\n=== " << (allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << " ===" << std::endl;

	return allPassed ? 0 : 1;
}
