#ifndef DATASETSYNCER_H
#define DATASETSYNCER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QString>
#include <json/json.h>

#include "databaseconnectioninfo.h"

class DataSet;

class DataSetSyncer : public QObject
{
	Q_OBJECT
public:
	DataSetSyncer(DataSet * dataSet, QObject * parent = nullptr);
	~DataSetSyncer();

	void					startFileSyncing(const QString & filePath);
	void					stopFileSyncing();
	bool					isFileSyncing()								const { return _fileWatcher && _fileWatcher->files().size() > 0; }

	void					startDatabaseSyncing(const Json::Value & dbJson, bool syncImmediately = false);
	void					stopDatabaseSyncing();
	bool					isDatabaseSyncing()							const { return _dbInfo && _dbInfo->synching(); }
	const Json::Value &		databaseJson()								const { return _databaseJson; }

	void					syncNow();

signals:
	void					syncingStarted(int dataSetId);
	void					syncingFinished(int dataSetId, bool success);
	void					syncingProgress(int dataSetId, int percent);

	void					askUserForRelink(int dataSetId);
	QString					askPassword(int dataSetId, QString title, QString message);
	bool					askYesNo(int dataSetId, QString title, QString message);
	void					showWarning(int dataSetId, QString title, QString message);

private slots:
	void					fileChanged(const QString & path);
	void					databaseSyncIntervalPassed();

private:
	void					doSync();

	DataSet				*	_dataSet			= nullptr;
	QFileSystemWatcher	*	_fileWatcher		= nullptr;
	DatabaseConnectionInfo	*	_dbInfo		= nullptr;
	Json::Value				_databaseJson		= Json::nullValue;
	bool					_isSyncing			= false;
};

#endif
