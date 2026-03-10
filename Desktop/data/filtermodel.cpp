#include <QMap>
#include "filtermodel.h"
#include "datasetpackage.h"
#include "filter.h"
#include "qutils.h"

FilterModel::FilterModel(QObject * parent)
	: QObject(parent)
{

	connect(DataSetPackage::pkg(), &DataSetPackage::shownDataSetChanged,	this, &FilterModel::filterChanged);
	connect(DataSetPackage::pkg(), &DataSetPackage::filtersCountChanged,	this, &FilterModel::filterDropDownListChanged,	Qt::QueuedConnection);
	connect(DataSetPackage::pkg(), &DataSetPackage::shownFilterChanged,		this, &FilterModel::filterChanged									);
	connect(DataSetPackage::pkg(), &DataSetPackage::shownFilterChanged,		this, &FilterModel::filterDropDownListChanged,	Qt::QueuedConnection);
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

void FilterModel::resetRFilter()
{
	if (filter()->defaultRFilter() != filter()->rFilter())
		UndoStack::singleton()->pushCommand(new SetRFilterCommand(filter(), filter()->defaultRFilter()));
}


void FilterModel::processFilterResult(QString name)
{
	if(!filter()) 
		return; //Cause there probably is no data anyway then
	
	if(filter()->nameQ() ==  name)
	{
		filter()->checkFilterResults();
		return;
	}
	
	Filter * f = DataSetPackage::pkg()->dataSet()->filter(fq(name));
	
	if(!f)
		f = DataSetPackage::pkg()->dataSet()->createFilter(fq(name), false);
	
	f->checkFilterResults();
	
}

void FilterModel::processFilterErrorMsg()
{
	if(!filter())
		return;
	
	filter()->checkFilterResults();
	
	if(filter()->errorMsg() != "")
	{
		setFilterVisible(true);
		
		//Now this might be caused by some labelfilter, or something. However, the filterwindow by default does not show the generatedFilter
		//Maybe its better to open it on the R display then?
		if(isJustGeneratedFilter())
			setShowEasyFilter(false);
	}
}

void FilterModel::onFilterChanged()
{
	if(filter())
		setCurrentFilterId(filter()->id());
}

void FilterModel::computeColumnSucceeded(QString columnName, QString, bool dataChanged)
{
	if(dataChanged && filter()->columnUsed(columnName))
		filter()->setInvalidated(true);
}

QVariantList FilterModel::_filterDropDownList(bool forCustomMenu) const
{
	typedef QMap<QString, QVariant> localMap;
	
	QVariantList out;
	
	if(DataSetPackage::pkg()->workspace())
	{
		if(forCustomMenu)
			out.append(localMap{std::make_pair("value", tq("---")), std::make_pair("label", "---")});
		
		for(DataSet * dataSet : DataSetPackage::pkg()->workspace()->dataSets())
		{
			out.append(localMap{std::make_pair("value", tq(dataSet == DataSetPackage::pkg()->dataSet() ? "*" : "-")), std::make_pair("label", dataSet->title() + ":")});
			
			if(dataSet->defaultFilter())
				out.append(localMap{std::make_pair("value", tq(std::to_string(dataSet->defaultFilter()->id()))), std::make_pair("label", dataSet->defaultFilter()->title())});
			
			for(const Filter * f : dataSet->filters())
				if(f != dataSet->defaultFilter())
					out.append(localMap{std::make_pair("value", tq(std::to_string(f->id()))), std::make_pair("label", f->title())});
			
			if(forCustomMenu)
				out.append(localMap{std::make_pair("value", tq("---")), std::make_pair("label", "---")});
			else
				out.append(localMap{std::make_pair("value", tq("---")), std::make_pair("label", tq(std::to_string(dataSet->id())))});
		}
	}
	
	return out;
}

bool FilterModel::filterVisible() const
{
	return _filterVisible;
}

void FilterModel::setFilterVisible(bool newFilterVisible)
{
	if (_filterVisible == newFilterVisible)
		return;
	_filterVisible = newFilterVisible;
		
	emit filterVisibleChanged();
}

bool FilterModel::showEasyFilter() const
{
	return _showEasyFilter;
}

void FilterModel::setShowEasyFilter(bool newShowEasyFilter)
{
	if (_showEasyFilter == newShowEasyFilter)
		return;
	_showEasyFilter = newShowEasyFilter;
	emit showEasyFilterChanged();
}

void FilterModel::reset()
{
	_showEasyFilter = true;
	_filterVisible  = false;
}

QString FilterModel::currentFilter() const
{
	return !filter() ? "" : tq(filter()->name());
}

int FilterModel::currentFilterId() const
{
	return !filter() ? -1 : filter()->id();
}

QString FilterModel::currentFilterTitle() const
{
	return !filter() ? "" : filter()->title();
}

void FilterModel::setCurrentFilterId(int id)
{
	DataSetPackage::pkg()->workspace()->showFilter(id);
	
	emit filterChanged();
	emit filterDropDownListChanged();
	
	DataSetPackage::pkg()->workspace()->refresh();
	
}

void FilterModel::renameCurrentFilter(const QString &newName)
{
	DataSetPackage::pkg()->dataSet()->shownFilter()->setName(fq(newName));
	emit filterChanged();
	emit filterDropDownListChanged();
}

void FilterModel::deleteCurrentFilter()
{
	DataSetPackage::pkg()->dataSet()->deleteShownFilter();
	emit filterChanged();
	emit filterDropDownListChanged();
}

void FilterModel::addFilter(int dataSetId)
{
	DataSet * dataSet = dataSetId == -1 
			? DataSetPackage::pkg()->dataSet() 
			: DataSetPackage::pkg()->workspace() 
			  ? DataSetPackage::pkg()->workspace()->dataSetById(dataSetId) 
			  : nullptr;
	
	if(dataSet)
		dataSet->addFilter();
}
