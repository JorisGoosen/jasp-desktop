#include "datasetsyncer.h"
#include "dataset.h"
#include "log.h"

#include <QFileInfo>

DataSetSyncer::DataSetSyncer(DataSet * dataSet, QObject * parent)
	: QObject(parent), _dataSet(dataSet)
{
}

DataSetSyncer::~DataSetSyncer()
{
	stopFileSyncing(true);
	stopDatabaseSyncing(true);
	
}

void DataSetSyncer::startFileSyncing(const QString & filePath)
{
	QFileInfo fi(filePath);
	if(!fi.exists())
	{
		Log::log() << "DataSetSyncer::startFileSyncing: File does not exist: " << filePath.toStdString() << std::endl;
		return;
	}

	QString absPath = fi.absoluteFilePath();

	if(_fileWatcher && _fileWatcher->files().contains(absPath))
	{
		_dataSet->setDataFile(absPath.toStdString(), fi.lastModified().toSecsSinceEpoch());
		return;
	}

	if(!_fileWatcher)
	{
		_fileWatcher = new QFileSystemWatcher(this);
		connect(_fileWatcher, &QFileSystemWatcher::fileChanged, this, &DataSetSyncer::fileChanged);
	}
	else if(!_fileWatcher->files().isEmpty())
		_fileWatcher->removePaths(_fileWatcher->files());

	_fileWatcher->addPath(absPath);
	_dataSet->setDataFile(absPath.toStdString(), fi.lastModified().toSecsSinceEpoch());
	_dataSet->setDataFileSynch(true);
}

void DataSetSyncer::stopFileSyncing(bool isExit)
{
	if(_fileWatcher)
	{
		_fileWatcher->removePaths(_fileWatcher->files());
		delete _fileWatcher;
		_fileWatcher = nullptr;
	}

	if(!isExit)
		_dataSet->setDataFileSynch(false);
}

void DataSetSyncer::startDatabaseSyncing(const Json::Value & dbJson, bool syncImmediately)
{
	_databaseJson = dbJson;

	stopDatabaseSyncing();

	if(dbJson == Json::nullValue)
		return;

	_dbInfo = new DatabaseConnectionInfo(dbJson, this);

	connect(_dbInfo, &DatabaseConnectionInfo::synchingIntervalPassed,	this, &DataSetSyncer::databaseSyncIntervalPassed);
	connect(_dbInfo, &DatabaseConnectionInfo::askPassword, [this](QString title, QString message) -> QString {
		return emit askPassword(_dataSet->id(), title, message);
	});
	connect(_dbInfo, &DatabaseConnectionInfo::showYesNo, [this](QString title, QString message) -> bool {
		return emit askYesNo(_dataSet->id(), title, message);
	});
	connect(_dbInfo, &DatabaseConnectionInfo::showWarning, [this](QString title, QString message) {
		emit showWarning(_dataSet->id(), title, message);
	});

	_dataSet->setDatabaseJson(dbJson.toStyledString());

	if(_dbInfo->_interval > 0)
		_dbInfo->startSynching(syncImmediately);
}

void DataSetSyncer::stopDatabaseSyncing(bool isExit)
{
	if(_dbInfo)
	{
		_dbInfo->stopSynching();
		_dbInfo->deleteLater();
		_dbInfo = nullptr;
	}

	if(!isExit)
		_dataSet->setDatabaseJson(Json::nullValue);
}

void DataSetSyncer::syncNow()
{
	if(isDatabaseSyncing())
		databaseSyncIntervalPassed();
	else if(isFileSyncing() && !_fileWatcher->files().isEmpty())
		fileChanged(_fileWatcher->files().first());
	else
		emit askUserForRelink(_dataSet->id());
}

void DataSetSyncer::fileChanged(const QString & path)
{
	QFileInfo fi(path);
	if(!fi.exists())
	{
		Log::log() << "DataSetSyncer: Synced file no longer exists: " << path.toStdString() << std::endl;
		return;
	}

	long newTimestamp = fi.lastModified().toSecsSinceEpoch();
	if(newTimestamp <= _dataSet->dataFileTimestamp())
		return;

	_dataSet->setDataFile(path.toStdString(), newTimestamp);

	doSync();
}

void DataSetSyncer::databaseSyncIntervalPassed()
{
	doSync();
}

void DataSetSyncer::doSync()
{
	if(!_dataSet || _isSyncing)
		return;

	_isSyncing = true;
	int id = _dataSet->id();
	emit syncingStarted(id);

	QString locator;
	QString extension;
	QString dbJson;

	if(_databaseJson != Json::nullValue)
	{
		locator		= QString::fromStdString(_dataSet->dataFilePath());
		extension	= "DATABASE";
		dbJson		= QString::fromStdString(_databaseJson.toStyledString());
	}
	else
	{
		locator		= QString::fromStdString(_dataSet->dataFilePath());
		extension	= QFileInfo(locator).suffix();
	}

	emit syncRequired(id, locator, extension, dbJson);

	_isSyncing = false;
}

void DataSetSyncer::setSyncingResult(bool success)
{
	emit syncingFinished(_dataSet ? _dataSet->id() : -1, success);
}
