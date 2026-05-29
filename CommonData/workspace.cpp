#include <QCoreApplication>
#include "workspace.h"
#include "qutils.h"
#include "log.h"
#include "undostack.h"

Workspace * Workspace::_singleton = nullptr;

Workspace::Workspace(QObject *parent)
	: DataSetBaseNode{dataSetBaseNodeType::workspace, parent},
	  _varInfo(new VariableInfo(nullptr, this))
{
	assert(!_singleton);
	_singleton = this;
	
	connect(this, &Workspace::dataSetSynchingStart, this, &Workspace::synchingStart);
	connect(this, &Workspace::dataSetSynchingDone,	this, &Workspace::synchingDone);
}

Workspace::~Workspace()
{
	assert(_singleton == this);
	
	for(auto & idData : _dataSets)
		unregisterNode(idData.second);
	
	_dataSets.clear();
	
	_singleton = nullptr;
}


DatabaseInterface &Workspace::db()	
{ 
	return *DatabaseInterface::singleton(); 
}

const DatabaseInterface &Workspace::db() const
{ 
	return *DatabaseInterface::singleton(); 
}

QVariant Workspace::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
		return QVariant();
	
	if(index.row() >= rowCount() || index.column() >= columnCount())
		return QVariant(); // if there is no data then it doesn't matter what role we play
	
	DataSets sets = dataSets();
	DataSet * cur = sets[index.row()];

	switch(role)
	{
	case Qt::DisplayRole:									
	case int(dataPkgRoles::label):							 
	case int(dataPkgRoles::value):							return cur->descriptionQ();
	case int(dataPkgRoles::name):							return cur->name();//tq(cur->db().dataSetName(cur->id()));
	case int(dataPkgRoles::title):							return cur->title();
	case int(dataPkgRoles::description):					return cur->descriptionQ();
	}
	
	return QVariant();
}

void Workspace::setDataMode(bool mode)			
{
	if(_dataMode == mode)
		return;
	
	_dataMode = mode; 
	emit dataModeChanged(_dataMode);
	refresh();
}

void Workspace::setShowRSyntax(bool showRSyntax)					
{ 
	_showRSyntax		= showRSyntax;			
	dbUpdate();
	
	emit showRSyntaxChanged(_showRSyntax);
}

void Workspace::dbLoad(std::function<void(float)> progressCallback, Version doUpgradeFrom)
{
	intset	dataSets = db().dataSetIds();
	
	int		numLoaded = 0;
	
	for(int id : dataSets)
	{
		auto progressCallbackPerData = [&](float p)
		{
			float	d = dataSets.size(),
					i = 1.0 / d;
			
			progressCallback((numLoaded * i) + (p * i));	
		};
		
		
		_dataSets[id] = new DataSet(this, 0);
		_dataSets[id]->dbLoad(id, progressCallbackPerData, doUpgradeFrom);
		numLoaded++;
		
		if(!_shownDataSet)
			setShownDataSet(_dataSets[id]);
	}
	
	bool prev = _showRSyntax;
	db().workspaceLoad(_showRSyntax);
	
	if(prev != _showRSyntax)
		emit showRSyntaxChanged(_showRSyntax);
}

void Workspace::dbUpdate()
{
	db().workspaceUpdate(_showRSyntax);
}

void Workspace::dbDelete()
{
	for(auto & idData : _dataSets)
		idData.second->dbDelete();
	
	db().truncateAllTables();
}

//Should be merged with dbLoad() probably?
bool Workspace::checkForUpdates()
{
	intset	dataSets = db().dataSetIds(),
			missing;
	
	bool aChange = false;
	
	for(int id : dataSets)
		if(_dataSets.count(id))
		{
			if(_dataSets.at(id)->checkForUpdates())
				aChange = true;
		}
		else
		{
			_dataSets[id] = new DataSet(this, id);
			aChange = true;
			
			if(!_shownDataSet)
				_shownDataSet = _dataSets[id];
		}
	
	for(auto & idDataSet : _dataSets)
		if(!dataSets.count(idDataSet.first))
			missing.insert(idDataSet.first);
	
	for(int id : missing)
	{
		delete _dataSets[id];
		_dataSets.erase(id);
		aChange = true;
	}
	
	bool prev = _showRSyntax;
	db().workspaceLoad(_showRSyntax);
	
	if(prev != _showRSyntax)
		emit showRSyntaxChanged(_showRSyntax);
	
	return aChange;
}


DataSet *Workspace::shownDataSet() const
{
	return _shownDataSet;
}

void Workspace::setShownDataSet(DataSet *dataSet)
{
	if(_shownDataSet == dataSet)
		return;
	
	assert(dataSet->workspace() == this);
	
	disconnect(_shownDataSet, &DataSet::shownColumnChanged, this, &Workspace::shownColumnChanged);
	disconnect(_shownDataSet, &DataSet::shownFilterChanged, this, &Workspace::shownFilterChanged);
	
	_shownDataSet = dataSet;
	
	UndoStack::setCurrent(_shownDataSet->undoStack());
	
	connect(_shownDataSet, &DataSet::shownColumnChanged, this, &Workspace::shownColumnChanged, Qt::UniqueConnection);
	connect(_shownDataSet, &DataSet::shownFilterChanged, this, &Workspace::shownFilterChanged, Qt::UniqueConnection);
	
	_varInfo->setProvider(_shownDataSet->shownFilter());
			
	emit shownDataSetChanged(_shownDataSet);
	refresh();
}

void Workspace::setShownDataSet(int dataSetId)
{
	if(_dataSets.count(dataSetId))
		setShownDataSet(_dataSets.at(dataSetId));
	else
		Log::log() << "setShownDataSet(" << dataSetId << ") can't find the dataSet!" << std::endl;
}

void Workspace::deleteShownDataSet()
{
	if(!_shownDataSet)
		return;
	
	_dataSets.erase(_shownDataSet->id());
	_shownDataSet->dbDelete();
	UndoStack::setCurrent(nullptr);
	delete _shownDataSet;
		
	_shownDataSet = nullptr;
	
	DataSet * newShown = nullptr;
	
	for(auto & idDataSet : _dataSets)
	{
		newShown = idDataSet.second;
		break;
	}
	
	if(newShown)	setShownDataSet(newShown);
	else			refresh();
}

void Workspace::showFilter(int id)
{
	Filter * f = filterById(id);
	
	if(f)
	{
		setShownDataSet(f->data());
		f->data()->showFilter(f);
		_varInfo->setProvider(f);
		refresh();
	}
}

void Workspace::onShownFilterChanged(DataSet *dataSet)
{
	setShownDataSet(dataSet);
	emit shownFilterChanged();
}

void Workspace::refreshAllCompCols(Filter *f)
{
	assert(f);
	
	f->data()->invalidateAllComputedColumns();
}

void Workspace::setShownDataSet(QString name)
{
	for(auto & idData : _dataSets)
		if(idData.second->name() == name)
		{
			setShownDataSet(idData.second);
			return;
		}
}

DataSets Workspace::dataSets() const
{
	DataSets out;
	
	for(auto & idData : _dataSets)
		out.push_back(idData.second);
	
	return out;
}

DataSet *Workspace::dataSetById(int id) const
{
	if(_dataSets.count(id))
		return _dataSets.at(id);
	
	return nullptr;
}

Filter *Workspace::filterById(int id) const
{
	for(auto & idDataSet : _dataSets)
		if(idDataSet.second->filter(id))
			return idDataSet.second->filter(id);
	return nullptr;
}

Column *Workspace::shownColumn() const
{
	return shownDataSet() ? shownDataSet()->shownColumn() : nullptr;
}

Filter *Workspace::shownFilter() const
{
	return shownDataSet() ? shownDataSet()->shownFilter()	: nullptr;
}

void Workspace::setShownColumn(Column *newShownColumn)
{
	newShownColumn->data()->setShownColumn(newShownColumn); // Will also set shownDataSet en passant
}

void Workspace::setShownFilter(Filter *newShownFilter)
{
	newShownFilter->data()->showFilter(newShownFilter);
	setShownDataSet(newShownFilter->data());
}


DataSet * Workspace::createDataSet()
{
	bool shownDataSetExistsAndIsEmpty = 
			_shownDataSet && 
			(_shownDataSet->columnCount() == 0 // Simple case
			|| (_shownDataSet->columnCount() == 1 && _shownDataSet->rowCount() == 1 && _shownDataSet->data(_shownDataSet->index(0, 0)) == QVariant())); //Single empty cell
	
	if(shownDataSetExistsAndIsEmpty)
		return _shownDataSet;

	DataSet * newSet = new DataSet(this);

	if(!_shownDataSet)
		setShownDataSet(newSet);

	_dataSets[newSet->id()] = newSet;

	emit filtersCountChanged(); //Triggers filterDropDownListChanged in filtermodel

	return newSet;
}

Column *Workspace::createComputedColumn(const std::string &name, int dataSetId, int analysisId, columnType type, computedColumnType desiredType)
{
	if(_dataSets.count(dataSetId))
		return _dataSets.at(dataSetId)->createComputedColumn(name, type, desiredType, analysisId);
	
	return nullptr;
}

void Workspace::refresh()
{
	thread_local int refreshDepth = 0;
	
	beginResetModel();
	
	if(refreshDepth++ == 0)
	{
		for(auto & idData : _dataSets)
			idData.second->refresh();
		
		emit dataModeChanged(dataMode());
		emit showRSyntaxChanged(showRSyntax());
		emit shownDataSetChanged(shownDataSet());
		emit shownColumnChanged();
		emit shownFilterChanged();
	}
	refreshDepth--;
	endResetModel();
}


void Workspace::initializeComputedColumns()
{
	for(auto & idDataSet : _dataSets)
		for(Column * col : idDataSet.second->columns())
			col->checkForDependentColumnsToBeSent();
}

void Workspace::updateComputedColumnDependenciesForAnalysis(int analysisId, const stringset & usedVariables)
{
	for(DataSet * dataSet : dataSets())
		for(Column * col : dataSet->columns())
			if(col->isComputedByAnalysis(analysisId))
				col->setDependsOn(usedVariables);
}
