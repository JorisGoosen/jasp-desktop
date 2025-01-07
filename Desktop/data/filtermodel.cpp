#include "timers.h"
#include "filtermodel.h"
#include "jsonutilities.h"
#include "columnencoder.h"
#include "datasetpackage.h"
#include "models/filterq.h"


FilterModel::FilterModel(QObject * parent)
	: QObject(parent)
{


}

QString FilterModel::rFilter()			const	{ return !filter() ? defaultRFilter()		: tq(filter()->rFilter());					}
QString FilterModel::constructorR()		const	{ return !filter() ? ""						: tq(filter()->constructorR());				}
QString FilterModel::filterErrorMsg()	const	{ return !filter() ? ""						: tq(filter()->errorMsg());					}
QString FilterModel::generatedFilter()	const	{ return !filter() ? DEFAULT_FILTER_GEN		: tq(filter()->generatedFilter());			}
QString FilterModel::constructorJson()	const	{ return !filter() ? DEFAULT_FILTER_JSON	: tq(filter()->constructorJson());			}

void FilterModel::applyConstructorJson(QString newConstructorJson)
{
	if (newConstructorJson != constructorJson())
		UndoStack::singleton()->pushCommand(new SetJsonFilterCommand(filter(), newConstructorJson));
}

void FilterModel::applyRFilter(QString newRFilter)
{
	if (newRFilter != rFilter())
		UndoStack::singleton()->pushCommand(new SetRFilterCommand(filter(), newRFilter));
}


void FilterModel::processFilterResult()
{
	if(!filter())
		return;

	//Load new filter values from database
	if(filter()->dbLoadResultAndError())
	{
		emit filterErrorMsgChanged();
		emit refreshAllAnalyses();
		emit filterUpdated();
		updateStatusBar();
	}
}

void FilterModel::runFilter()
{
	filter()->setInvalidated(true);
	Also run it somehow!
}

void FilterModel::computeColumnSucceeded(QString columnName, QString, bool dataChanged)
{
	if(dataChanged && filter()->columnUsed(columnName))
		runFilter();
}

