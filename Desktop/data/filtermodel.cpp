#include <QMap>
#include "filtermodel.h"
#include "datasetpackage.h"
#include "workspace.h"
#include "filter.h"
#include "qutils.h"

FilterModel::FilterModel(QObject * parent)
	: QObject(parent)
{

	connect(Workspace::singleton(), &Workspace::shownDataSetChanged,	this, &FilterModel::filterChanged);
	connect(Workspace::singleton(), &Workspace::filtersCountChanged,	this, &FilterModel::filterDropDownListChanged,	Qt::QueuedConnection);
	connect(Workspace::singleton(), &Workspace::shownFilterChanged,		this, &FilterModel::filterChanged									);
	connect(Workspace::singleton(), &Workspace::shownFilterChanged,		this, &FilterModel::filterDropDownListChanged,	Qt::QueuedConnection);
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
	
	Filter * f = nullptr;
	for(Filter * fltr : DataSetPackage::pkg()->dataSet()->filters())
		if (fltr->name() == fq(name))
		{
			f = fltr;
			break;
		}
	
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
	auto makeMap = [](const QString & value, const QString & label) {
		QVariantMap m;
		m["value"] = value;
		m["label"] = label;
		return QVariant::fromValue(m);
	};
	
	QVariantList out;
	
	if(Workspace::singleton())
	{
		if(forCustomMenu)
			out.append(makeMap(tq("---"), QString("---")));
		
		for(DataSet * dataSet : Workspace::singleton()->dataSets())
		{
			out.append(makeMap(tq(dataSet == DataSetPackage::pkg()->dataSet() ? "*" : "-"), dataSet->titleQ() + ":"));
			
			if(dataSet->defaultFilter())
				out.append(makeMap(tq(std::to_string(dataSet->defaultFilter()->id())), dataSet->defaultFilter()->title()));
			
			for(const Filter * f : dataSet->filters())
				if(f != dataSet->defaultFilter())
					out.append(makeMap(tq(std::to_string(f->id())), f->title()));
			
			if(forCustomMenu)
				out.append(makeMap(tq("---"), QString("---")));
			else
				out.append(makeMap(tq("---"), tq(std::to_string(dataSet->id()))));
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
	Workspace::singleton()->showFilter(id);
	
	emit filterChanged();
	emit filterDropDownListChanged();
	
	Workspace::singleton()->refresh();
	
}

void FilterModel::renameCurrentFilter(const QString &newName)
{
	DataSetPackage::pkg()->dataSet()->shownFilter()->setName(fq(newName));
	emit filterChanged();
	emit filterDropDownListChanged();
}

void FilterModel::deleteCurrentFilter()
{
	if(DataSetPackage::pkg()->dataSet()->shownFilter())
		DataSetPackage::pkg()->dataSet()->shownFilter()->dbDelete();
	emit filterChanged();
	emit filterDropDownListChanged();
}

void FilterModel::addFilter(int dataSetId)
{
	DataSet * dataSet = dataSetId == -1 
			? DataSetPackage::pkg()->dataSet() 
			: Workspace::singleton() 
			  ? Workspace::singleton()->dataSetById(dataSetId) 
			  : nullptr;
	
	if(dataSet)
		new Filter(dataSet);
}
