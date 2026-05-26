#include "testsync.h"
#include "datasetsyncer.h"
#include "syncworker.h"
#include "dataset.h"
#include "workspace.h"
#include "databaseinterface.h"
#include "tempfiles.h"
#include "processinfo.h"
#include "utilities/settings.h"
#include "utilities/qutils.h"

#include <QFile>
#include <QDir>
#include <QSignalSpy>
#include <QFileSystemWatcher>
#include <QSqlDatabase>

void TestSync::init()
{
	Settings::informSettingsThatThisIsATest();
	TempFiles::init(ProcessInfo::currentPID());

	if(!DatabaseInterface::singleton())
		new DatabaseInterface(true);
}

void TestSync::cleanup()
{
	delete _worker;
	_worker = nullptr;

	delete _syncer;
	_syncer = nullptr;

	delete _dataSet;
	_dataSet = nullptr;

	DatabaseInterface::singleton()->close();
	DatabaseInterface::singleton()->closeInterfaces();
}

void TestSync::testDataSetSyncerFileSync()
{
	_dataSet = new DataSet();
	QVERIFY2(_dataSet->id() > 0, "DataSet should have a valid ID");

	_syncer = new DataSetSyncer(_dataSet);

	QVERIFY2(!_syncer->isFileSyncing(), "Should not be syncing initially");

	QTemporaryFile tempFile;
	tempFile.open();
	tempFile.write("a,b,c\n1,2,3\n");
	tempFile.flush();
	QString tempPath = tempFile.fileName();
	tempFile.close();

	_syncer->startFileSyncing(tempPath);

	QVERIFY2(_syncer->isFileSyncing(), "Should be file syncing after startFileSyncing");
	QVERIFY2(_dataSet->dataFileSynch(), "DataSet should have dataFileSynch set");
	QVERIFY2(!_dataSet->dataFilePath().empty(), "DataSet should have dataFilePath set");

	_syncer->stopFileSyncing();
	QVERIFY2(!_syncer->isFileSyncing(), "Should not be syncing after stopFileSyncing");
	QVERIFY2(!_dataSet->dataFileSynch(), "DataSet should not have dataFileSynch after stop");
}

void TestSync::testDataSetSyncerDatabaseSync()
{
	_dataSet = new DataSet();
	QVERIFY2(_dataSet->id() > 0, "DataSet should have a valid ID");

	_syncer = new DataSetSyncer(_dataSet);

	QVERIFY2(!_syncer->isDatabaseSyncing(), "Should not be database syncing initially");

	Json::Value dbJson;
	dbJson["dbType"]	= int(DbType::QSQLITE);
	dbJson["database"]	= ":memory:";
	dbJson["query"]		= "SELECT 1";
	dbJson["interval"]	= 0;

	_syncer->startDatabaseSyncing(dbJson, false);

	QVERIFY2(_syncer->isDatabaseSyncing(), "Should be database syncing");
	QVERIFY2(!_dataSet->databaseJson().empty(), "DataSet should have databaseJson set");

	_syncer->stopDatabaseSyncing();
	QVERIFY2(!_syncer->isDatabaseSyncing(), "Should not be syncing after stop");
	QVERIFY2(_dataSet->databaseJson().empty(), "DataSet databaseJson should be cleared");
}

void TestSync::testSyncWorkerProcessRequest()
{
	_worker = new SyncWorker(this);

	SyncRequest request;
	request.dataSetId	= 42;
	request.locator		= "test.csv";
	request.extension	= ".csv";

	QSignalSpy startedSpy(_worker, &SyncWorker::syncStarted);
	QSignalSpy completedSpy(_worker, &SyncWorker::syncCompleted);
	QSignalSpy progressSpy(_worker, &SyncWorker::syncProgress);

	_worker->processSyncRequest(request);

	QCOMPARE(startedSpy.count(),	1);
	QCOMPARE(startedSpy.at(0).at(0).toInt(), 42);

	QCOMPARE(completedSpy.count(),	1);
	QCOMPARE(completedSpy.at(0).at(0).toInt(), 42);
	QCOMPARE(completedSpy.at(0).at(1).toBool(), true);

	QVERIFY2(progressSpy.count() >= 1, "Should have progress updates");
}

void TestSync::testMultipleDataSetsSync()
{
	_dataSet = new DataSet();
	QVERIFY2(_dataSet->id() > 0, "DataSet should have a valid ID");

	DataSet * dataSet2 = new DataSet();
	QVERIFY2(dataSet2->id() > 0, "Second DataSet should have a valid ID");

	_syncer = new DataSetSyncer(_dataSet);
	DataSetSyncer * syncer2 = new DataSetSyncer(dataSet2);

	QTemporaryFile tempFile1, tempFile2;
	tempFile1.open();
	tempFile1.write("x,y\n1,2\n");
	tempFile1.flush();
	QString path1 = tempFile1.fileName();
	tempFile1.close();

	tempFile2.open();
	tempFile2.write("a,b\n3,4\n");
	tempFile2.flush();
	QString path2 = tempFile2.fileName();
	tempFile2.close();

	_syncer->startFileSyncing(path1);
	syncer2->startFileSyncing(path2);

	QVERIFY2(_syncer->isFileSyncing(), "First DataSet syncer should be active");
	QVERIFY2(syncer2->isFileSyncing(), "Second DataSet syncer should be active");

	QVERIFY2(_dataSet->dataFileSynch(), "First DataSet should have synch enabled");
	QVERIFY2(dataSet2->dataFileSynch(), "Second DataSet should have synch enabled");

	_syncer->stopFileSyncing();
	syncer2->stopFileSyncing();

	QVERIFY2(!_syncer->isFileSyncing(), "First syncer should be stopped");
	QVERIFY2(!syncer2->isFileSyncing(), "Second syncer should be stopped");

	delete syncer2;
	delete dataSet2;
}

QTEST_MAIN(TestSync)
