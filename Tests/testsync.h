#ifndef TESTSYNC_H
#define TESTSYNC_H

#include <QObject>
#include <QTest>
#include <QTemporaryFile>

class DataSetSyncer;
class DataSet;
class SyncWorker;

class TestSync : public QObject
{
	Q_OBJECT

private slots:
	void init();
	void cleanup();

	void testDataSetSyncerFileSync();
	void testDataSetSyncerDatabaseSync();
	void testSyncWorkerProcessRequest();
	void testMultipleDataSetsSync();

private:
	DataSet		*	_dataSet	= nullptr;
	DataSetSyncer*	_syncer		= nullptr;
	SyncWorker	*	_worker		= nullptr;
};

#endif
