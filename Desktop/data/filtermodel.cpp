#include <QMap>
#include "filtermodel.h"
#include "datasetpackage.h"
#include "filter.h"
#include "qutils.h"

FilterModel::FilterModel(QObject * parent)
	: QObject(parent)
{

	connect(DataSetPackage::pkg(), &DataSetPackage::DataSetChanged, this, &FilterModel::filterChanged, Qt::QueuedConnection);
}

Filter *FilterModel::filter() const
{
	return DataSetPackage::filter();
}


bool FilterModel::isJustGeneratedFilter() const
{
	return filter() && filter()->rFilter() == Filter::defaultRFilter() && filter()->constructorJson() == DEFAULT_FILTER_JSON;
}

void FilterModel::applyConstructorJson(QString newConstructorJson)
{
	if (newConstructorJson != filter()->constructorJson())
		UndoStack::singleton()->pushCommand(new SetJsonFilterCommand(filter(), newConstructorJson));
}

void FilterModel::applyRFilter(QString newRFilter)
{
	if (newRFilter != filter()->rFilter())
		UndoStack::singleton()->pushCommand(new SetRFilterCommand(filter(), newRFilter));
}


void FilterModel::processFilterResult()
{
	if(!filter())
		return;
	
	filter()->checkFilterResults();
}

void FilterModel::processFilterErrorMsg()
{
	if(!filter())
		return;
	
	filter()->checkFilterResults();
}

void FilterModel::computeColumnSucceeded(QString columnName, QString, bool dataChanged)
{
	if(dataChanged && filter()->columnUsed(columnName))
		filter()->setInvalidated(true);
}

QVariantList FilterModel::filterDropDownList() const
{
	typedef QMap<QString, QVariant> localMap;
	
	QVariantList out = { localMap({std::make_pair("value", ""), std::make_pair("label", QObject::tr("No filter"))}) };
	
	//Right now we only have 1 filter, but later we can add support here for multiple filters, or of filters from analyses (such as from ListModelFilteredDataEntry)
	if(DataSetPackage::filter())
		out.append(localMap{std::make_pair("value", tq(DataSetPackage::filter()->name())), std::make_pair("label", QObject::tr("Use filter"))});
	
	return out;
}
