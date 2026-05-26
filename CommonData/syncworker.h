#ifndef SYNCWORKER_H
#define SYNCWORKER_H

#include <QObject>
#include <QString>
#include <functional>
#include <string>
#include <vector>
#include <map>

#include "common.h"

class DataSet;

struct SyncRequest
{
	int			dataSetId		= -1;
	std::string	locator;			// file path or database JSON string
	std::string	extension;			// file extension or "DATABASE"
};

class SyncWorker : public QObject
{
	Q_OBJECT
public:
	SyncWorker(QObject * parent = nullptr);
	~SyncWorker();

public slots:
	void	processSyncRequest(const SyncRequest & request);

signals:
	void	syncStarted(int dataSetId);
	void	syncProgress(int dataSetId, int percent);
	void	syncCompleted(int dataSetId, bool success);
	void	syncError(int dataSetId, QString error);

	bool	askUserYesNo(int dataSetId, QString title, QString message);
};

#endif
