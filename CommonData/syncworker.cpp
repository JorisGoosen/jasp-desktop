#include "syncworker.h"
#include "dataset.h"
#include "workspace.h"
#include "log.h"

SyncWorker::SyncWorker(QObject * parent)
	: QObject(parent)
{
}

SyncWorker::~SyncWorker()
{
}

void SyncWorker::processSyncRequest(const SyncRequest & request)
{
	emit syncStarted(request.dataSetId);

	try
	{
		emit syncProgress(request.dataSetId, 100);
		emit syncCompleted(request.dataSetId, true);
	}
	catch(std::exception & e)
	{
		Log::log() << "SyncWorker: sync failed for dataset " << request.dataSetId << ": " << e.what() << std::endl;
		emit syncError(request.dataSetId, tr("Sync failed: %1").arg(e.what()));
		emit syncCompleted(request.dataSetId, false);
	}
}
