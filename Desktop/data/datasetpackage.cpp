//
// Copyright (C) 2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "datasetpackage.h"
#include "log.h"
#include "qutils.h"
#include <QThread>
#include "timers.h"
#include "utils.h"
#include "columnencoder.h"
#include "utilities/appdirs.h"
#include "engine/enginesync.h"
#include "gui/preferencesmodel.h"
#include "utilities/messageforwarder.h"
#include "databaseconnectioninfo.h"
#include "filtermodel.h"
#include <ranges>
#include "variableinfo.h"
#include "fileevent.h"


DataSetPackage * DataSetPackage::_singleton = nullptr;

DataSetPackage::DataSetPackage(QObject * parent) : QObject(parent)
{
	if(_singleton) throw std::runtime_error("DataSetPackage can be constructed only once!");
	_singleton = this;
	//True init is done in setEngineSync!
	
	_db			= new DatabaseInterface(true);

	createWorkspace();
	
	connect(this, &DataSetPackage::isModifiedChanged,					this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::loadedChanged,						this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::currentFileChanged,					this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::folderChanged,						this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::isModifiedAfterAutoSaveChanged,		this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::currentFileChanged,					this, &DataSetPackage::nameChanged);
	connect(this, &DataSetPackage::dataModeChanged,						this, &DataSetPackage::onDataModeChanged);
	
	connect(PreferencesModel::prefs(), &PreferencesModel::autoSaveAtAllChanged,			this, &DataSetPackage::handleAutoSavePrefChange);
	connect(PreferencesModel::prefs(), &PreferencesModel::autoSaveIntervalSecChanged,	this, &DataSetPackage::handleAutoSavePrefChange);

	connect(&_doWalCheckPointTimer,		&QTimer::timeout, this, &DataSetPackage::doWalCheckPoint);
	connect(&_autoSaveTimer,			&QTimer::timeout, this, &DataSetPackage::handleAutoSave);
	
	_undoStack = new UndoStack(this);
	
	_doWalCheckPointTimer	.setInterval(5*60*1000);
	_doWalCheckPointTimer	.setSingleShot(false);
	_doWalCheckPointTimer	.start();
	
	
	_autoSaveTimer			.setSingleShot(false);
	handleAutoSavePrefChange();
}

DataSetPackage::~DataSetPackage() 
{ 
	_singleton = nullptr; 
}


void DataSetPackage::createWorkspace()
{
	assert(!_workspace);
	
	_workspace = new Workspace(this);
	
	_workspace->setShowRSyntax(PreferencesModel::prefs()->showRSyntaxInResults());
	
	connectWorkspace();
	
	emit workspaceChanged();
}

DataSet * DataSetPackage::createDataSet()
{
	JASPTIMER_SCOPE(DataSetPackage::createDataSet);
	
	//The assumption here is that a new DataSet is needed. But not that anything else needs to be destroyed.
	
	if(!_workspace)
		createWorkspace();
	
	DataSet * dataSet = workspace()->createDataSet();
		
	return dataSet;
}

void DataSetPackage::loadWorkspace(std::function<void(float)> progressCallback)
{
	if(workspace())
		deleteWorkspace(false); //no dbDelete necessary cause we just copied an old sqlite file here from the JASP file
	
	_db->close();
	stopEngines();
	_db->load();		
	_db->upgradeDBFromVersion(_jaspVersion);
	
	bool do019Upgrade = _jaspVersion < "0.19"; // A tweak needs to be made to the data as its loaded, see https://github.com/jasp-stats/jasp-desktop/pull/5367
	
	createWorkspace();
	
	workspace()->dbLoad(progressCallback, _jaspVersion);
	
	if (do019Upgrade)
	{
		// In 0.18.3 and before, there was a bug with the order of dataFilePath and description in the database.
		// dataFilePath was set empty and description has dataFilePath.
		if (dataSet()->dataFilePath().empty())
		{
			QFileInfo fileInfo(description());
			if (fileInfo.isFile())
				dataSet()->setDataFileQ(description());
		}
	}

	workspace()->initializeComputedColumns();

	refresh();
	
	restartEngines();
}

void DataSetPackage::deleteWorkspace(bool dbDeletePlease)
{
	JASPTIMER_SCOPE(DataSetPackage::deleteWorkspace);
	
	if(dbDeletePlease)
		dbDelete();
	delete _workspace;
	_workspace = nullptr;
	_undoStack->clear();
	
	if(dbDeletePlease)
	{
		emit shownDataSetChanged(nullptr); //This can trigger models to read from DataSet and if we dont want dbDelete we most likely dont want this either
		emit workspaceChanged();
	}
}

void DataSetPackage::connectWorkspace()
{
	if(!workspace())
		return;
	
	Workspace		::connect(workspace(),	&Workspace::showWarning,						this,			&DataSetPackage::showWarning						);
	Workspace		::connect(workspace(),	&Workspace::showAnalysis,						this,			&DataSetPackage::showAnalysis						);
	Workspace		::connect(workspace(),	&Workspace::datasetChanged,						this,			&DataSetPackage::datasetChanged						);
	Workspace		::connect(workspace(),	&Workspace::somethingModified,					this,			&DataSetPackage::setModifiedFileMenu				);
	Workspace		::connect(workspace(),	&Workspace::dataModeChanged,					this,			&DataSetPackage::dataModeChanged					);
	Workspace		::connect(workspace(),	&Workspace::sendFilter,							this,			&DataSetPackage::sendFilter							);
	Workspace		::connect(workspace(),	&Workspace::sendFilterByName,					this,			&DataSetPackage::sendFilterByName					);
	Workspace		::connect(workspace(),	&Workspace::filtersCountChanged,				this,			&DataSetPackage::filtersCountChanged				);
	Workspace		::connect(workspace(),	&Workspace::shownFilterChanged,					this,			&DataSetPackage::shownFilterChanged					);
	Workspace		::connect(workspace(),	&Workspace::refreshAllAnalyses,					this,			&DataSetPackage::refreshAllAnalyses					);
	Workspace		::connect(workspace(),	&Workspace::enginesPrepareForData,				this,			&DataSetPackage::enginesPrepareForDataSignal		);
	Workspace		::connect(workspace(),	&Workspace::enginesReceiveNewData,				this,			&DataSetPackage::enginesReceiveNewDataSignal		);
	Workspace		::connect(workspace(),	&Workspace::shownDataSetChanged,				this,			&DataSetPackage::shownDataSetChanged				);	
	Workspace		::connect(workspace(),	&Workspace::dataSetSynchingStart,				this,			&DataSetPackage::beginLoadingData					);	
	Workspace		::connect(workspace(),	&Workspace::dataSetSynchingDone,				this,			&DataSetPackage::endLoadingData						);	
	Workspace		::connect(workspace(),	&Workspace::runComputedColumn,					this,			&DataSetPackage::runComputedColumn					);	
	Workspace		::connect(workspace(),	&Workspace::checkForDependentAnalyses,			this,			&DataSetPackage::checkForDependentAnalyses			);	
	Workspace		::connect(workspace(),	&Workspace::emptyValuesChanged,					this,			&DataSetPackage::workspaceEmptyValuesChanged		);	
	

	DataSetPackage	::connect(this,			&DataSetPackage::filterByNameDone,				workspace(),	&Workspace::filterByNameDone						);
	DataSetPackage	::connect(this,			&DataSetPackage::createDataSetBlockingQueued,	workspace(),	&Workspace::createDataSet,							Qt::BlockingQueuedConnection);
	
	
	emit shownDataSetChanged(nullptr);
	emit shownFilterChanged();
}


Filter * DataSetPackage::filter()
{
    return pkg()->workspace() && pkg()->workspace()->shownDataSet() ? pkg()->workspace()->shownDataSet()->shownFilter() : nullptr;
}

void DataSetPackage::setEngineSync(EngineSync * engineSync)
{
	_engineSync = engineSync;

	//These signals should *ONLY* be called from a different thread than _engineSync!
	connect(this,	&DataSetPackage::enginesPrepareForDataSignal,	_engineSync,	&EngineSync::enginesPrepareForData,	Qt::QueuedConnection);
	connect(this,	&DataSetPackage::enginesReceiveNewDataSignal,	_engineSync,	&EngineSync::enginesReceiveNewData,	Qt::QueuedConnection);


	reset();
}

bool DataSetPackage::isThisTheSameThreadAsEngineSync()
{
	return	_engineSync && QThread::currentThread() == _engineSync->thread();
}

void DataSetPackage::enginesPrepareForData()
{
	if(dataMode())
		return;

	if(isThisTheSameThreadAsEngineSync())	_engineSync->enginesPrepareForData();
	else									emit enginesPrepareForDataSignal();
}

void DataSetPackage::enginesReceiveNewData()
{
	if(!dataMode())
	{
		if(isThisTheSameThreadAsEngineSync())	_engineSync->enginesReceiveNewData();
		else									emit enginesReceiveNewDataSignal();
	}
}

void DataSetPackage::reset(bool newDataSet)
{
	Log::log() << "DataSetPackage::reset()" << std::endl;
	
	beginLoadingData();

	emit chooseColumn(-1); //Unselect any column in ColumnModel
	
	deleteWorkspace();

	if(newDataSet)	
		createDataSet();
	
	_archiveVersion				= Version();
	_jaspVersion				= Version();
	_analysesHTML				= QString();
	_analysesData				= Json::arrayValue;
	_warningMessage				= std::string();
	_hasAnalysesWithoutData		= false;
	_analysesHTMLReady			= false;
	_isJaspFile					= false;

	setLoaded(false);
	setModified(false);
	setCurrentFile("");

	endLoadingData();
}

///This function assumes there should afterwards be only 1 DataSet!
void DataSetPackage::generateEmptyData()
{
	bool wasAlreadyLoaded = isLoaded();

	if(workspace())
		deleteWorkspace();
	createWorkspace();
	
	DataSet * newSet = dataSet() ? dataSet() : createDataSet();
	
	newSet->setColumnCount(1);
	newSet->setRowCount(1, false);
	
	newSet->column(0)->initFromLookups(newSet->freeNewColumnName(0), 1, [](size_t){return "";}, [](size_t){return "";}, "", columnType::scale, {}, PreferencesModel::prefs()->thresholdScale(), PreferencesModel::prefs()->orderByValueByDefault());
	
	setModified(false);
	
	if(!wasAlreadyLoaded)
	{
		emit newDataLoaded();
	}
	
	newSet->resetAllFilters();
	newSet->setDataFileSynch(false);
	
	if(workspace()->shownDataSet() != newSet)
		workspace()->setShownDataSet(newSet);
	else
		workspace()->refresh();
}

void DataSetPackage::onDataModeChanged(bool dataMode)
{
	Log::log() << "Data Mode " << (dataMode ? "on" : "off") << "!" << std::endl;
	_dataMode = dataMode;
	
	doWalCheckPoint();

	beginResetModel();
	endResetModel();
	
	if(false)
		enginesReceiveNewData();
	
	if(workspace())
		workspace()->setDataMode(dataMode);
}

DataSetBaseNode * DataSetPackage::indexPointerToNode(const QModelIndex & index) const
{
	DataSetBaseNode * node = static_cast<DataSetBaseNode*>(index.internalPointer());
	
	//Below was when I was trying to use dataChanged for columnModel setData updates. but it got messy, so instead we do reset but we do not need to loop over all nodes anymore everytime we convert something
	return node;
	//Sometimes the proxymodels seem to return pointers to destroyed objects, so lets check even if it gives some overhead...
	//return dataSetBaseNodeStillExists(node) ? node : nullptr;
}

/// the following hierarchy is used (Where parents point to children):
/// QModelIndex()/root -> DataSet_N* Where each DataSet is located at row 0 and column N (0-based)
/// DataSet_N -> Data (0r,0c) and Filters(1r,0c)
/// Filters -> Filter_N* where column is filterIndex and row value-index in the filtered-bools
/// Data -> Directly to the data but each column (dfwith any row) can also be used as a parent for getting the Labels
/// Data[*r, Nc] -> Labels
QModelIndex DataSetPackage::index(int row, int column, const QModelIndex &parent) const
{
	const void * pointer = nullptr;

	if(parent.isValid()) //Top of hierarchy (parent != valid) has no pointer
	{
		if(!parent.internalPointer()) //Parent has no pointer stored so this must be a dataSet
		{
			//Currently we only have a single dataSet but in the future there will be more.
			//Then they will be differentiated here by row
			pointer = dynamic_cast<const void*>(_dataSet);
		}
		else
		{
			DataSetBaseNode * parentNode = indexPointerToNode(parent);
			
			switch(parentNode->nodeType())
			{
			case dataSetBaseNodeType::dataSet:
			{
				DataSet * data = dynamic_cast<DataSet*>(parentNode);
				// if row 0 it is "data" else "filters"
				pointer = row == 0 ? dynamic_cast<const void*>(data->dataNode()) : dynamic_cast<const void*>(data->filtersNode());
				break;
			}
				
			case dataSetBaseNodeType::data:
			{
				DataSet * data = dynamic_cast<DataSet*>(parentNode->parent());
				pointer = dynamic_cast<const void*>(data->column(column));
				break;
			}
				
			case dataSetBaseNodeType::filters:
			{
				//Later on we should support multiple filters here by selecting a filter per column
				DataSet * data = dynamic_cast<DataSet*>(parentNode->parent());
				pointer = dynamic_cast<const void*>(data->filter());
				break;
			}
				
			case dataSetBaseNodeType::column:
			{
				Column	* col	= dynamic_cast<Column*>(parentNode);
				Label	* lab	= col->labelByIndexNonEmpty(row);
				pointer			= dynamic_cast<const void*>(lab);
				break;
			}
				
			case dataSetBaseNodeType::label:	//Label & Filter cant be a parentnode
			case dataSetBaseNodeType::filter:
			default:
				throw std::runtime_error("Somehow a label, filter or unknown DataSetBaseNode was passed as parent for an index... This is not allowed.");
				break;
			}
		}
	}
	
	return createIndex(row, column, pointer);
}

///Used to get the parent for a DataSetPackageSubNodeModel
QModelIndex DataSetPackage::indexForSubNode(DataSetBaseNode * node) const
{
	if(node)
		switch(node->nodeType())
		{
		case dataSetBaseNodeType::dataSet:
			return createIndex(0, 0, dynamic_cast<void *>(_dataSet));

		case dataSetBaseNodeType::data:
			return createIndex(0, 0, dynamic_cast<void *>(_dataSet->dataNode()));

		case dataSetBaseNodeType::filters:
			return createIndex(1, 0, dynamic_cast<void *>(_dataSet->filtersNode()));

		case dataSetBaseNodeType::column:
		{
			Column * col = dynamic_cast<Column*>(node);
			if (col)
				return createIndex(0, col->data()->columnIndex(col), dynamic_cast<void *>(col));
			else
				return QModelIndex();
		}

		case dataSetBaseNodeType::label: //Doesnt really make sense to have this as the parent of a subnodemodel but whatever
		{
			Label	* lab = dynamic_cast<Label*>( node);
			Column	* col = lab ? dynamic_cast<Column*>(node->parent()) : nullptr;
			int		i = col ? col->labelIndexNonEmpty(lab) : -1;

			return createIndex(i, 0, dynamic_cast<void*>(lab));
		}

		case dataSetBaseNodeType::filter: //Doesnt really make sense to have this as the parent of a subnodemodel but whatever
		{
			return createIndex(0, 0, dynamic_cast<void*>(_dataSet->filter()));
		}

		default:
			break;
		}

	return QModelIndex();
}

QModelIndex DataSetPackage::parent(const QModelIndex & index) const
{
	if(!index.isValid())
		return QModelIndex();

	
	DataSetBaseNode * node = indexPointerToNode(index);

	if(!node)
		return QModelIndex();
	
	switch(node->nodeType())
	{
	case dataSetBaseNodeType::filters: [[fallthrough]];
	case dataSetBaseNodeType::dataSet:
		return QModelIndex();
		
	case dataSetBaseNodeType::data:
		return indexForSubNode(_dataSet);

	case dataSetBaseNodeType::column:
		return indexForSubNode(_dataSet->dataNode());
		
	case dataSetBaseNodeType::label:
	{
	//	Label	* label	= dynamic_cast<Label*>(node);
		Column	* col	= dynamic_cast<Column*>(node->parent());
		
		return indexForSubNode(col);
	}
		
		
	case dataSetBaseNodeType::filter:
		return indexForSubNode(_dataSet->filtersNode());
		
	default:
		break;
	}
	
	return QModelIndex(); //Shouldnt get here though
}

int DataSetPackage::rowCount(const QModelIndex & parent) const
{
	if(!parent.isValid())
		return 1; //There is only a "column" of DataSets as topnodes
	
	
	DataSetBaseNode * node = indexPointerToNode(parent);

	if(!node)
		return 1;
	
	switch(node->nodeType())
	{
	case dataSetBaseNodeType::dataSet:
		return 2; //data + filters
		
	case dataSetBaseNodeType::data:
	case dataSetBaseNodeType::filters:
	{
		DataSet * data = dynamic_cast<DataSet*>(node->parent());

		return data ? data->rowCount() : 0;
	}
		
	case dataSetBaseNodeType::column:
	{
		Column * col = dynamic_cast<Column*>(node);
		
		return !col ? 0 : col->labelsNonEmptyCount();
	}
		
	case dataSetBaseNodeType::filter:
	case dataSetBaseNodeType::label:
		return 1;
	
	case dataSetBaseNodeType::unknown:
		return 0;
	}

	return 0; // <- because gcc is stupid
}

int DataSetPackage::columnCount(const QModelIndex &parent) const
{
	if(!parent.isValid())
		return 1; //There is only a "row" of DataSets as topnodes, and currently a single column because a single DataSet	
	
	DataSetBaseNode * node = indexPointerToNode(parent);

	if(!node)
		return 1;
	
	switch(node->nodeType())
	{
	case dataSetBaseNodeType::dataSet:
	{
		return 1; //data + filters are on rows
	}
	case dataSetBaseNodeType::data:
	{
		DataSet * data = dynamic_cast<DataSet*>(node->parent());
		return data->columnCount();
	}
		
	case dataSetBaseNodeType::filters:
	{
		return 1; //change when implementing multiple filters
	}
		
	case dataSetBaseNodeType::column:
	{
		//Column * col = dynamic_cast<Column*>(node);
		
		return 1;
	}
		
	case dataSetBaseNodeType::filter:
	case dataSetBaseNodeType::label:

		return 1;
	
	case dataSetBaseNodeType::unknown:
		return 0;
	}

	return 0; // <- because gcc is stupid
}

bool DataSetPackage::getRowFilter(int row) const
{
	return !_dataSet ? false : data(this->index(row, 0, indexForSubNode(_dataSet->filtersNode()))).toBool();
}

QVariant DataSetPackage::getDataSetViewLines(bool up, bool left, bool down, bool right)
{
	return			(left ?		1 : 0) +
					(right ?	2 : 0) +
					(up ?		4 : 0) +
					(down ?		8 : 0);
}

int DataSetPackage::dataRowCount() const 
{ 
	return !_dataSet ? 0 : rowCount(indexForSubNode(_dataSet->dataNode()));
}

int DataSetPackage::dataColumnCount() const 
{ 
	return !_dataSet ? 0 : columnCount(indexForSubNode(_dataSet->dataNode()));
}

QVariant DataSetPackage::data(const QModelIndex &index, int role) const
{
    JASPTIMER_SCOPE(DataSetPackage::data);
    
	if(!index.isValid())
		return QVariant();

	if(role == int(specialRoles::selected))
		return false; //DataSetPackage doesnt know anything about selected, only ColumnModel does (now)

	DataSetBaseNode *	node		= indexPointerToNode(index);
//					*	parentNode	= !index.parent().isValid() ? nullptr : indexPointerToNode(index.parent());

	if(!node)
		return QVariant();// : QVariant(tq("DataSet_" + std::to_string(dynamic_cast<DataSet*>(node)->id())));
	
	switch(node->nodeType())
	{
	default:
		return QVariant();

	case dataSetBaseNodeType::filter:
	{
		Filter * filter = dynamic_cast<Filter*>(node);
		if(index.row() < 0 || index.row() >= int(filter->filtered().size()))
			return true;
		
		return  QVariant(filter->filtered()[index.row()]);
	}

	case dataSetBaseNodeType::column:
	{
		Column	* column	= dynamic_cast<Column*>(node);
		DataSet * dataSet	= column ? column->data() : nullptr;

		if(!dataSet || index.row() >= int(dataSet->rowCount()))
			return QVariant(); // if there is no data then it doesn't matter what role we play

		switch(role)
		{
		case Qt::DisplayRole:									return tq(column->getDisplay(index.row(), true, true));
		case int(specialRoles::noSepaDisplay):					return tq(column->getDisplay(index.row(), false, false));
		case int(specialRoles::label):							return tq(column->getLabel(index.row(), false, true));
		case int(specialRoles::value):							return tq(column->getValue(index.row()));
		case int(specialRoles::name):							return tq(column->name());
		case int(specialRoles::title):							return tq(column->title());
		case int(specialRoles::filter):							return getRowFilter(index.row());
		case int(specialRoles::columnType):						return int(column->type());
		case int(specialRoles::description):					return tq(column->description());
		case int(specialRoles::inEasyFilter):					return isColumnUsedInEasyFilter(column->name());
		case int(specialRoles::shadowDisplay):					return tq(column->getShadow(index.row()));
		case int(specialRoles::valuesDblList):					return getColumnValuesAsDoubleList(getColumnIndex(column->name()));
		case int(specialRoles::nonFilteredNumericValuesCount):	return column->nonFilteredNumericsCount();
        case int(specialRoles::nonFilteredLevels):				return tq(column->nonFilteredLevels());
		case int(specialRoles::computedColumnType):				return int(column->codeType());
		case int(specialRoles::columnPkgIndex):					return index.column();
		case int(specialRoles::lines):
		{
			bool	iAmActive		= getRowFilter(index.row()),
					belowMeIsActive = index.row() < column->rowCount() - 1	&& data(this->index(index.row() + 1, index.column(), index.parent()), int(specialRoles::filter)).toBool();

			return getDataSetViewLines(
				iAmActive,
				iAmActive,
				iAmActive && !belowMeIsActive,
				iAmActive && index.column() == columnCount(index.parent()) - 1 //always draw left line and right line only if last col
			);
		}
		}
	}
		
	case dataSetBaseNodeType::label:
	{
		int			parRowCount = rowCount(index.parent());
		Column	*	column		= dynamic_cast<Column*>(node->parent());

		if(!_dataSet || index.row() >= parRowCount)
			return QVariant(); // if there is no data then it doesn't matter what role we play

		switch(role)
		{
		case int(specialRoles::nonFilteredNumericValuesCount):	return column->nonFilteredNumericsCount();
        case int(specialRoles::nonFilteredLevels):				return tq(column->nonFilteredLevels());
		case int(specialRoles::valuesDblList):					return getColumnValuesAsDoubleList(getColumnIndex(column->name()));
		case int(specialRoles::description):
		case int(specialRoles::filter):
		{
			Label * label = dynamic_cast<Label*>(indexPointerToNode(index));
			if (!label)	return QVariant();
			return (role == int(specialRoles::description)) ? QVariant(tq(label->description())) : QVariant(label->filterAllows());
		}
		case int(specialRoles::value):							return tq(column->labelByIndexNonEmpty(index.row())->originalValueAsString());
		case int(specialRoles::lines):							return getDataSetViewLines(index.row() == 0, index.column() == 0, true, true);
		case int(specialRoles::label):							[[fallthrough]];
		case Qt::DisplayRole:									return tq(column->labelByIndexNonEmpty(index.row())->labelDisplay());
		default:												return QVariant();
		}
	}
	}

	return QVariant(); // <- because gcc is stupid
}

qsizetype DataSetPackage::getMaximumColumnWidthInCharacters(int columnIndex) const
{
	return _dataSet ? _dataSet->getMaximumColumnWidthInCharacters(columnIndex) : 0;
}

QVariant DataSetPackage::headerData(int section, Qt::Orientation orientation, int role)	const
{
	if (!_dataSet || section < 0 || section >= (orientation == Qt::Horizontal ? dataColumnCount() : dataRowCount()))
		return QVariant();
    
    JASPTIMER_SCOPE(DataSetPackage::headerData);

	if(orientation == Qt::Vertical)
		switch(role)
		{
		default:
			return QVariant();

		case int(specialRoles::maxRowHeaderString):
			return QString::number(_dataSet->rowCount()) + "XXX";

		case Qt::DisplayRole:
			return QVariant(section + 1);
		}
	else
	{
		Column * col = _dataSet ? _dataSet->column(section) : nullptr;
		
		switch(role)
		{
		case int(specialRoles::maxColString):
		{
			//calculate some kind of maximum string to give views an expectation of the width needed for a column
			bool		hasFilter	= col && (col->hasFilter() || isColumnUsedInEasyFilter(col->name()));
			QString		dummyText	= headerData(section, orientation, int(specialRoles::maxColumnHeaderString)).toString() + (isColumnComputed(section) ? "XXX" : "") + (hasFilter ? "XXX" : ""); //Bit of padding for hamburger, filtersymbol and columnIcon
			qsizetype	colWidth	= getMaximumColumnWidthInCharacters(section);

			while(colWidth > dummyText.length())
				dummyText += "X";

			return dummyText;
		}
		case int(specialRoles::maxColumnHeaderString):			return headerData(section, orientation, Qt::DisplayRole).toString() + "XXX";
		case int(specialRoles::maxRowHeaderString):				return QString::number(_dataSet ? _dataSet->rowCount() : 0 )		+ "XXX";
		case Qt::TextAlignmentRole:								return QVariant(Qt::AlignCenter);
		case int(specialRoles::filter):							return		!col ? false							: col->hasFilter() || isColumnUsedInEasyFilter(col->name());
		case int(specialRoles::name):							[[fallthrough]];
		case Qt::DisplayRole:									return tq(	!col ? "?"								: col->name());
		case int(specialRoles::labelsHasFilter):				return		!col ? false							: col->hasFilter();
		case int(specialRoles::columnIsComputed):				return		!col ? false							: col->isComputed() && col->codeType() != computedColumnType::analysisNotComputed;
		case int(specialRoles::computedColumnError):			return tq(	!col ? "?"								: col->error());
		case int(specialRoles::computedColumnIsInvalidated):	return		!col ? false							: col->invalidated();
		case int(specialRoles::columnType):						return int(	!col ? columnType::unknown				: col->type());
		case int(specialRoles::computedColumnType):				return int(	!col ? computedColumnType::notComputed	: col->codeType());
		case int(specialRoles::description):					return tq(	!col ? "?"								: col->description());
		case int(specialRoles::title):							return tq(	!col ? "?"								: col->title());
		case int(specialRoles::previewScale):
		case int(specialRoles::previewOrdinal):					
		case int(specialRoles::previewNominal):					
		{
			columnType colTypeWanted = 
					role == int(specialRoles::previewNominal) 
					? columnType::nominal 
					: role == int(specialRoles::previewOrdinal)
					? columnType::ordinal
					: columnType::scale;
			
			stringvec	preview		= !col ? stringvec()	: col->previewTransform(colTypeWanted);
			
			if(preview.size() != 4)
				return "";
			
			QString	levelsTotal		= tq(preview[0]),
					levelsNums		= tq(preview[1]),
					vals			= tq(preview[2]),
					empties			= tq(preview[3]);
			
			return 	(colTypeWanted == columnType::scale 
					?	tr("There are %1 total levels, of which %2 have a numeric value.\nAs a '%3' it looks like: %4\n%5")
						.arg(levelsTotal)
						.arg(levelsNums)
						.arg(VariableInfo::getTypeFriendly(colTypeWanted))
						.arg(vals)
						.arg(
							empties == "" 
							? "" 
							: tr("Implicit missing values: %1").arg(empties)
						)
						
					:	tr("There are %1 total levels.\nAs a '%2' it looks like: %3")
					.arg(levelsTotal)
					.arg(VariableInfo::getTypeFriendly(colTypeWanted))
					.arg(vals));
		}
		}
	}

	return QVariant();
}

bool DataSetPackage::setData(const QModelIndex &index, const QVariant &value, int role)
{
    JASPTIMER_SCOPE(DataSetPackage::setData);
    
	if(_waitingForLanguageChange || !index.isValid() || !_dataSet) return false;

	DataSetBaseNode * node = indexPointerToNode(index);
	
	if(!node)
		return false;

	switch(node->nodeType())
	{
	default:
		return false;

	case dataSetBaseNodeType::column:
		if(node)
		{    
			JASPTIMER_SCOPE(DataSetPackage::setData Column);

			Column	* column	= dynamic_cast<Column*>(node);
			//DataSet * data		= column->data();

			if(role == Qt::DisplayRole || role == Qt::EditRole || role == int(specialRoles::value) || role == int(specialRoles::valueLabelPair) || role == int(specialRoles::valuesStrList))
			{				
				bool				isPair	= role == int(specialRoles::valueLabelPair),
									isVals	= role == int(specialRoles::valuesStrList);
				QVariantList		listVar	= isPair || isVals ? value.toList()	: QVariantList{ value };
				bool				aChange = false;
				
				if(!isVals)
				{
					const std::string	val		= fq(listVar[0].toString()),
										label	= fq(isPair ? listVar[1].toString() : "");
										aChange	= !isPair	
												? column->setStringValue(index.row(), val == EmptyValues::displayString() ? "" : val)
												: column->setValue(index.row(), val, label);
				}
				else //Its a list of values, for instance "intial values"
				{
					int r=0;
					for(const QVariant & val : listVar)
						if(column->setStringValue(index.row() + r++, fq(val.toString() == tq(EmptyValues::displayString()) ? "" : val.toString())))
							aChange = true;
				}
				
				if(aChange)
				{
						JASPTIMER_SCOPE(DataSetPackage::setData reset model);

						setManualEdits(true); //Don't synch with external file after editing
						
						column->labelsRemoveOrphans();
						column->nonFilteredCountersReset();
						column->labelsHandleAutoSort();

						stringvec	changedCols = {column->name()};
	
						refresh();
						emit datasetChanged(tq(changedCols), {}, {}, false, false);
						emit labelsReordered(tq(column->name()));
						
						if(column->hasFilter())
						{
							emit labelFilterChanged();
							emit runFilter();
						}
				}
				
				return true;
			}
			else
			{
				bool aChange = false;

				switch(role)
				{
				case int(specialRoles::description):
					column->setDescription(value.toString().toStdString());
					aChange = true;
					break;

				case int(specialRoles::title):
					column->setTitle(value.toString().toStdString());
					aChange = true;
					break;

				case int(specialRoles::columnType):
					if(value.toInt() >= int(columnType::unknown) && value.toInt() <= int(columnType::scale))
					{
						columnType converted = static_cast<columnType>(value.toInt());
						if(converted != column->type() && setColumnType(index.column(), converted))
						{
							aChange = true;
							emit columnDataTypeChanged(tq(column->name()));
						}
					}
					break;
				}

				if(aChange)
				{
					beginResetModel();
					endResetModel();
					setManualEdits(true);
				}
				return true;
			}
		}
		else
			return false;
	
	case dataSetBaseNodeType::label:
	{
		JASPTIMER_SCOPE(DataSetPackage::setData Label);
		
		Column * column = dynamic_cast<Column*>(node->parent());
		
		int parColCount = columnCount(index.parent()),
			parRowCount = rowCount(index.parent());

		if(!_dataSet || index.column() >= parColCount || index.row() >= parRowCount || index.column() < 0 || index.row() < 0)
			return false;

		switch(role)
		{
		case int(specialRoles::filter):
			if(value.typeId() != QMetaType::Bool) 
				return false;

			setManualEdits(true);
			return setLabelAllowFilter(index, value.toBool());

		case int(specialRoles::description):
			setManualEdits(true);
			return setLabelDescription(index, value.toString());

		case int(specialRoles::value):
			return setLabelValue(index,  value.toString());

		case int(specialRoles::label):
			return setLabelDisplay(index, value.toString());
			
		default:
			return false;
		}
	}
	}

	return false;
}


void DataSetPackage::resetFilterAllows(size_t columnIndex)
{
	if(!_dataSet) return;

	_dataSet->column(columnIndex)->resetFilter();

	emit labelFilterChanged();

	QModelIndex parentModel = indexForSubNode(_dataSet->dataNode());
	emit dataChanged(DataSetPackage::index(0, columnIndex,	parentModel),	DataSetPackage::index(rowCount() - 1, columnIndex, parentModel), {int(specialRoles::filter)} );

	parentModel = indexForSubNode(_dataSet->column(columnIndex));
	emit dataChanged(DataSetPackage::index(0, 0,	parentModel),			DataSetPackage::index(rowCount(parentModel) - 1, columnCount(parentModel) - 1, parentModel), {int(specialRoles::filter)} );


	emit filteredOutChanged(columnIndex);
}

bool DataSetPackage::setLabelDescription(const QModelIndex & index, const QString & newDescription)
{
	Label		*	label	= dynamic_cast<Label*>(indexPointerToNode(index));
	Column		*	column	= dynamic_cast<Column*>(label->parent());
	QModelIndex		parent	= index.parent();
	
	if(!column || index.row() > rowCount(parent))
		return false;

	label->setDescription(newDescription.toStdString());
	
	emit dataChanged(DataSetPackage::index(index.row(), 0, parent),	DataSetPackage::index(index.row(), columnCount(parent), parent), {int(specialRoles::description)});	//Emit dataChanged for filter

	return true;
}

bool DataSetPackage::setLabelDisplay(const QModelIndex &index, const QString &newLabel)
{
	Label			*	label		= dynamic_cast<Label*>(indexPointerToNode(index));
	Column			*	column		= dynamic_cast<Column*>(label->parent());
	QModelIndex			parent		= index.parent();
	stringvec			changedCols	;
	bool				aChange		= false,
						setManual	= false;
	
	if(!column || index.row() > rowCount(parent))
		return false;
	
	beginSynchingData(false);
	
	if(label->setLabel(newLabel.toStdString()))
	{
		aChange = true;
		
		if(dataFileCanHaveLabels())
			setManual = true;
	}
	
	if(aChange)
		changedCols = {column->name()};
	
	endSynchingDataChangedColumns(changedCols, false, false);

	if(setManual)
		setManualEdits(true);

	return aChange;
}

bool DataSetPackage::setLabelValue(const QModelIndex &index, const QString &newLabelValue)
{
	Label			*	label		= dynamic_cast<Label*>(indexPointerToNode(index));
	Column			*	column		= dynamic_cast<Column*>(label->parent());
	QModelIndex			parent		= index.parent();
	stringvec			changedCols	;
	bool				aChange		= false,
						aNumber		= false;
	
	if(!column || index.row() > rowCount(parent))
		return false;
	
	beginSynchingData(false);
	
	Json::Value originalValue = newLabelValue.toStdString();

	int		anInteger;
	double	aDouble;

	if(	(aNumber =	ColumnUtils::getDoubleValue(newLabelValue.toStdString(), aDouble))	)	originalValue = aDouble;
	if(				ColumnUtils::getIntValue(	newLabelValue.toStdString(), anInteger)	)	originalValue = anInteger;
	
	
	{
		// Here we will overwrite the original value with the new origval.
		// but if the label is the same as the original value we want to make the users life easier and replace it as well.
		// this makes sense if the user is changing a string or number. But if the user is recoding, so turning values from str => dbl
		// then we dont want to do this, because then the label should be different afterwards.
		
		//summarized:
		// if orgval == label then: 
		// if (oldorigval == dbl && newOrigVal == dbl) || (olorigval != dbl && newOrigVal != dbl)  then replace both
		// if neworigval == dbl and oldorigval != dbl then replace only value
		
		// But only if we are allowed to change both because of https://github.com/jasp-stats/INTERNAL-jasp/issues/2680 (allow editing of only value/label and disable the other one for computed columns
		// which means that if this column is a computed column of scale type we are only allowed to change the label and only the value for the other types.
		// so in this case this means that if it is a computed column, and of type !scale we do *not* also update the label when updating the value. Because otherwise it would override the data from the computed column...
		
		bool dontSetLabel = label->originalValueAsString(false) != label->label() || (originalValue.isDouble() && !label->originalValue().isDouble());
		
		if(!dontSetLabel && column->isComputed() && column->type() != columnType::scale)
			dontSetLabel = true;
		
		if(dontSetLabel)	aChange = label->setOriginalValue(originalValue)	||	aChange;
		else				aChange = label->setOrigValLabel(originalValue)		||	aChange;
	}
	
	column->labelsHandleAutoSort();
	
	if(aChange)
		changedCols = {column->name()};

	endSynchingDataChangedColumns(changedCols, false, false);
	
	if(aChange)
		setManualEdits(true); //A value change is a manual edit for sure as that changes the data itself

	return aChange;
}

bool DataSetPackage::setLabelAllowFilter(const QModelIndex & index, bool newAllowValue)
{
	JASPTIMER_SCOPE(DataSetPackage::setAllowFilterOnLabel);
	
	Label			*	label		= dynamic_cast<Label*>(indexPointerToNode(index));
	Column			*	column		= dynamic_cast<Column*>(label->parent());

	if(!column)
		return false;

	if (label->filterAllows() == newAllowValue) //Did not change!
		return true;

	bool atLeastOneRemains = newAllowValue;

	QModelIndex parent	= index.parent();
	size_t		row		= index.row();
	
	if(int(row) > rowCount(parent))
		return false;

	const Labels	& labels = column->labels();

	if(!atLeastOneRemains) //Do not let the user uncheck every single one because that is useless, the user wants to uncheck row so lets see if there is another one left after that.
		for(size_t i=0; i< labels.size(); i++)
		{
			if(labels[i] != label && !labels[i]->isEmptyValue() && labels[i]->filterAllows())
			{
				atLeastOneRemains = true;
				break;
			}
		}
	
	atLeastOneRemains = atLeastOneRemains || column->labelsNonEmptyCount() > labels.size();

	if(atLeastOneRemains)
	{
		int col = column->data()->columnIndex(column);

		bool before = column->hasFilter();
		label->setFilterAllows(newAllowValue);
		
		notifyColumnFilterStatusChanged(col); //basically resetModel now

		emit labelFilterChanged();
		QModelIndex columnParentNode = indexForSubNode(column);
		//emit dataChanged(DataSetPackage::index(row, 0, columnParentNode),	DataSetPackage::index(row, columnCount(columnParentNode), columnParentNode), { int(specialRoles::filter) });
		emit filteredOutChanged(col);
		
		if(column->dropLevels() == dropLevelsType::noChoice && !newAllowValue) //No choice was made yet, but the user disabled a label, so I guess they dont want all labels
		{
			column->setDropLevels(dropLevelsType::drop);
			//To be sure everything is updated:
			emit refreshAllCompCols();
			emit refreshAllAnalyses();
		}

		return true;
	}
	else
		return false;
}

int DataSetPackage::filteredOut(size_t col) const
{
	if(!_dataSet || int(col) >= dataColumnCount())
		return 0; //or -1?

	const Column * column = _dataSet->column(col);
	if(!column)
		return 0;

	const Labels &	labels		= column->labels();
	int			filteredOut = 0;

	for(const Label * label : labels)
		if(!label->filterAllows())
			filteredOut++;

	return filteredOut;
}

Qt::ItemFlags DataSetPackage::flags(const QModelIndex &index) const
{
	const auto *	node		= indexPointerToNode(index);
	bool			isDataNode	= node && (node->nodeType() == dataSetBaseNodeType::data || node->nodeType() == dataSetBaseNodeType::column),
					isEditable	= !isDataNode || (_dataMode && !isColumnComputed(index.column()));

	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | (isEditable ? Qt::ItemIsEditable : Qt::NoItemFlags);
}

QHash<int, QByteArray> DataSetPackage::roleNames() const
{
	static bool						set = false;
	static QHash<int, QByteArray> roles = QAbstractItemModel::roleNames ();

	if(!set)
	{
		for(const auto & enumString : dataPkgRolesToStringMap())
			roles[int(enumString.first)] = QString::fromStdString(enumString.second).toUtf8();

		set = true;
	}

	return roles;
}
}

void DataSetPackage::setModified(bool value)
{
	if ((!value || _isLoaded || _hasAnalysesWithoutData) && value != _isModified)
	{
		_isModified = value;
		emit isModifiedChanged();
	}
	
	setModifiedAfterAutoSave(_isModified);
}

void DataSetPackage::setModifiedAfterAutoSave(bool value)
{
	if (value != _isModifiedAfterAutoSave)
	{
		_isModifiedAfterAutoSave = value;
		emit isModifiedAfterAutoSaveChanged();
	}
	
	if(_isModifiedAfterAutoSave)
		_autoSaveTimer.start(); //Restart the timer so that no one gets an annoying save right in the middle of doing something. Yet there will be a save when you dont do something for a while
}


void DataSetPackage::handleAutoSave()
{
	if(_isModifiedAfterAutoSave)				
		emit makeAnAutoSave();
	
	else if(FileEvent::autoSaveExists() && _isModified)
			Utils::touch(fq(FileEvent::pathTmp()));
}


void DataSetPackage::setLoaded(bool loaded)
{
	if(loaded == _isLoaded)
		return;

	_isLoaded						= loaded;

	emit loadedChanged();
}

QString DataSetPackage::description() const
{
	return tq(dataSet() ? dataSet()->description() : "");
}

void DataSetPackage::setDescription(const QString &description)
{
	if (!dataSet()) return;
	
	dataSet()->setDescription(fq(description));

	emit descriptionChanged();
}

void DataSetPackage::prepareForLanguageChange()
{
	_waitingForLanguageChange = true; //Dont accept changes while the interface changes
}

void DataSetPackage::languageChangeDone()
{
	_waitingForLanguageChange = false; //Dont accept changes while the interface changes

	dataSet()->refresh();
}

void DataSetPackage::handleAutoSavePrefChange()
{
	_autoSaveTimer.setInterval(1000 * PreferencesModel::prefs()->autoSaveIntervalSec());
	
	if(_autoSaveTimer.isActive() != PreferencesModel::prefs()->autoSaveAtAll())
	{
		if(!PreferencesModel::prefs()->autoSaveAtAll())		
			_autoSaveTimer.stop();
		else
			_autoSaveTimer.start();
	}
}


void DataSetPackage::doWalCheckPoint()
{
	if(DatabaseInterface::singleton())
		DatabaseInterface::singleton()->doWalCheckPoint();
}


void DataSetPackage::refreshColumn(QString columnName)
{
	if(dataSet())
	{
		dataSet()->column(columnName)->refresh();
		refresh(); //Hopefully trigger sortfilterproxymodel model reconstruction
	}
}


void DataSetPackage::columnWasOverwritten(const std::string & columnName, const std::string &)
{
	dataSet()->emitColumnChanged(tq(columnName));
}


void DataSetPackage::refresh()
{
	if(!dataSet())
		return;
	
	dataSet()->refresh();
}




void DataSetPackage::beginLoadingData(bool)
{
	JASPTIMER_SCOPE(DataSetPackage::beginLoadingData);

	enginesPrepareForData();
	doWalCheckPoint();
}

void DataSetPackage::stopEngines()
{
	if(EngineSync::singleton()) //During testing this may be false
		EngineSync::singleton()->stopEngines();
}

void DataSetPackage::restartEngines()
{
	if(EngineSync::singleton()) //During testing this may be false
		EngineSync::singleton()->restartEngines();
}



void DataSetPackage::endLoadingData(bool)
{
	JASPTIMER_SCOPE(DataSetPackage::endLoadingData);

	Log::log() << "DataSetPackage::endLoadingData" << std::endl;
	
	doWalCheckPoint();
	enginesReceiveNewData();
	
	refresh();
}

void DataSetPackage::dbDelete()
{
	JASPTIMER_SCOPE(DataSetPackage::dbDelete);
	if(dataSet() && dataSet()->id() != -1)
		dataSet()->dbDelete();
}

int DataSetPackage::thresholdScale()
{
	return PreferencesModel::prefs()->thresholdScale();
}

int DataSetPackage::orderByValueByDefault()
{
	return PreferencesModel::prefs()->orderByValueByDefault();
}

void DataSetPackage::resetVariableTypes()
{
	if(workspace())
		for(DataSet * dataSet : workspace()->dataSets())
			dataSet->resetVariableTypes(PreferencesModel::prefs()->thresholdScale());
}


stringvec DataSetPackage::getColumnNames()
{
	return _dataSet ? _dataSet->getColumnNames() : stringvec();
}

std::map<std::string,columnType> DataSetPackage::getColumnTypesMap()
{
	return _dataSet ? _dataSet->getColumnTypesMap() : std::map<std::string,columnType>();
}

bool DataSetPackage::isColumnDifferentFromStringLookUps(const std::string & columnName, const std::string & title, size_t rows,	const std::function<std::string(size_t)> valueLookup, const std::function<std::string(size_t)> labelLookup, const stringset & strEmptyVals)
{
	Column * col = _dataSet->column(columnName);
	
	if(col)
		return col->isColumnDifferentFromStringLookUps(title, rows, valueLookup, labelLookup, strEmptyVals);

	return true;
}

void DataSetPackage::renameColumn(const std::string & oldColumnName, const std::string & newColumnName)
{
	try
	{
		Column * col = _dataSet->column(oldColumnName);
		col->setName(newColumnName);
	}
	catch(...)
	{
		Log::log() << "Couldn't rename column from '" << oldColumnName << "' to '" << newColumnName << "'" << std::endl;
	}
}

void DataSetPackage::writeDataSetToOStream(std::ostream & out, bool includeComputed)
{
	std::vector<const Column*> cols;

	//Add a UTF-8 BOM
	out.put(0xEF);
	out.put(0xBB);
	out.put(0xBF);

	int columnCount = _dataSet->columnCount();

	for (int i = 0; i < columnCount; i++)
	{
		Column		*	column	= _dataSet->column(i);

		if(!column->isComputed() || includeComputed)
			cols.push_back(column);
	}


	for (size_t i = 0; i < cols.size(); i++)
	{
		const Column *	column	= cols[i];
		std::string		name	= column->name();

		if (stringUtils::escapeValue(name))	out << '"' << name << '"';
		else								out << name;

		if (i < cols.size()-1)	out << ",";
		else					out << "\n";

	}

	size_t rows = _dataSet->rowCount();

	for (size_t r = 0; r < rows; r++)
		for (size_t i = 0; i < cols.size(); i++)
		{
			const Column *column = cols[i];

			std::string value = column->getValue(r);
			if (value != "")
			{
				if (stringUtils::escapeValue(value))	out << '"' << value << '"';
				else									out << value;
			}

			if (i < cols.size()-1)		out << ",";
			else if (r != rows-1)		out << "\n";
		}
}


stringvec DataSetPackage::getColumnDataStrs(size_t columnIndex)
{
	JASPTIMER_SCOPE(DataSetPackage::getColumnDataStrs);

	if(_dataSet == nullptr)
		return {};

	Column * col = _dataSet->column(columnIndex);
	
	stringvec out;
	out.reserve(_dataSet->rowCount());
	
	for(size_t r=0; r<col->rowCount(); r++)
	{
		std::string value = col->getValue(r);	
		out.push_back(value != "." ? value : "");
	}

	return out;
}

void DataSetPackage::setColumnName(size_t columnIndex, const std::string & newName)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	if (!column)
		return;

	std::string oldName = getColumnName(columnIndex);

	if(column->setName(newName))
	{
		setManualEdits(true);
		emit datasetChanged({}, {}, QMap<QString, QString>({{tq(oldName), tq(newName)}}), false, false);
		enginesReceiveNewData();
	}
	refresh(); //We do refresh in any case because then the emptied name of the column in variableswindow will get filled again
}

void DataSetPackage::setColumnTitle(size_t columnIndex, const std::string & newTitle)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	if (!column)
		return;

	column->setTitle(newTitle);
	
	refresh();
}

void DataSetPackage::setColumnComputeFilter(size_t columnIndex, const std::string & newFilter)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	
	if (!column)
		return;

	column->setComputeFilter(newFilter);
	
	refresh();
}

void DataSetPackage::setColumnDescription(size_t columnIndex, const std::string & newDescription)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	if (!column)
		return;

	column->setDescription(newDescription);
	
	refresh();
}

void DataSetPackage::setColumnComputedType(size_t columnIndex, computedColumnType type)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	if (!column)
		return;

	column->setCodeType(type);
}

void DataSetPackage::setColumnDropLevels(size_t columnIndex, dropLevelsType dropLevels)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	if (!column)
		return;

	column->setDropLevels(dropLevels);

	refresh();
	
	emit refreshAllCompCols();
	emit refreshAllAnalyses();
}

void DataSetPackage::setColumnHasLabels(size_t columnIndex, bool hasLabels)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	
	if (!column)
		return;

	column->setHasLabels(hasLabels);
	
	refresh();
	emit labelsReordered(tq(column->name()));
	
}


void DataSetPackage::setColumnHasCustomEmptyValues(size_t columnIndex, bool hasCustomEmptyValue)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	if (!column || column->hasCustomEmptyValues() == hasCustomEmptyValue)
		return;
	
	column->setHasCustomEmptyValues(hasCustomEmptyValue);
	
	refresh();
	emit datasetChanged({tq(column->name())}, {}, {}, false, false);
}

void DataSetPackage::setColumnCustomEmptyValues(size_t columnIndex, const stringset& customEmptyValues)
{
	if(!_dataSet)
		return;

	Column* column = _dataSet->column(columnIndex);
	
	if(column && column->setCustomEmptyValues(customEmptyValues))
	{
		refresh();
		
		emit datasetChanged({tq(column->name())}, {}, {}, false, false);
	}
}

void DataSetPackage::columnsReverseValues(intset columnIndexes)
{
	columnsApply(columnIndexes, [&](Column * column) 
	{ 
		column->valuesReverse();
		return true;
	});
}

void DataSetPackage::columnsSetAutoSortForColumns(std::map<int,bool> sortPerColumn)
{
	intset cols;
	for(auto colSort : sortPerColumn)
		cols.insert(colSort.first);
	
	columnsApply(cols, [&](Column * column, int colIdx) 
	{ 
		column->setAutoSortByValue(sortPerColumn[colIdx]);
		return true;
	});
	
	if(cols.size() == 1)
		emit chooseColumn(*cols.begin());
}

void DataSetPackage::columnsApply(intset columnIndexes, std::function<bool(Column * column, int col)> applyThis)
{
	if(!_dataSet)
		return;
	
	QStringList changedCols;

	for(int columnIndex : columnIndexes)
	{
		Column* column = _dataSet->column(columnIndex);
	
		if(column)
		{
			if(applyThis(column, columnIndex))
				changedCols << tq(column->name());
		}	
	}
	
	if(changedCols.size() > 0)
	{
		refresh();
		emit datasetChanged(changedCols, {}, {}, false, false);
	}
}

void DataSetPackage::columnsApply(intset columnIndexes, std::function<bool(Column * column)> applyThis)
{
	columnsApply(columnIndexes, [&](Column * column, int){ return applyThis(column); });
}

columnType DataSetPackage::getColumnType(size_t columnIndex) const
{
	return _dataSet && _dataSet->column(columnIndex) ? _dataSet->column(columnIndex)->type() : columnType::unknown;
}

columnType DataSetPackage::getColumnType(const QString& name) const
{
	return _dataSet ? getColumnType(_dataSet->getColumnIndex(fq(name))) : columnType::unknown;
}


std::string DataSetPackage::getColumnName(size_t columnIndex) const
{
	return _dataSet && _dataSet->column(columnIndex) ? _dataSet->column(columnIndex)->name() : "";
}

QStringList DataSetPackage::getColumnLabelsAsStringList(size_t columnIndex)	const
{
	return tq(getColumnLabelsAsStrVec(columnIndex));
}


std::map<std::string, bool> DataSetPackage::getColumnFilterAllows(size_t columnIndex) const
{
	std::map<std::string, bool> map;
	if(columnIndex < 0 || columnIndex >= dataColumnCount()) 
		return map;
	
	Column * column =_dataSet->columns()[columnIndex];
	
	for (const Label * label : column->labels())
		if(!label->isEmptyValue())
			map[label->label()] = label->filterAllows();
	
	return map;
}

stringvec DataSetPackage::getColumnLabelsAsStrVec(size_t columnIndex) const
{
	stringvec list;
	if(columnIndex < 0 || columnIndex >= dataColumnCount()) 
		return list;

	return _dataSet->columns()[columnIndex]->labelsAsStrings();
}


stringvec DataSetPackage::getColumnLevelsAsStrVec(size_t columnIndex) const
{
	stringvec list;
	if(columnIndex < 0 || columnIndex >= dataColumnCount()) 
		return list;

	return _dataSet->columns()[columnIndex]->nonEmptyLevelsStrings();
}


QList<QVariant> DataSetPackage::getColumnValuesAsDoubleList(size_t columnIndex)	const
{
	QList<QVariant> list;
	if(columnIndex < 0 || columnIndex >= dataColumnCount()) return list;

	for (double value : _dataSet->columns()[columnIndex]->dbls())
		list.append(value);

	return list;
}

bool DataSetPackage::labelNeedsFilter(size_t columnIndex) const
{
	if(columnIndex < 0 || columnIndex >= dataColumnCount()) 
		return false;
			
	return _dataSet->columns()[columnIndex]->hasFilter();
}


void DataSetPackage::labelMoveRows(size_t colIdx, std::vector<size_t> rows, bool up)
{
	Column	*	column		= _dataSet->columns()[colIdx];
	sizetset	rowsChanged = column->labelsMoveRows(rows, up);
	
	if(rowsChanged.size())
	{
		QModelIndex p = indexForSubNode(column);
		emit dataChanged(index(0, 0, p), index(rowCount(p) - 1 , columnCount(p) - 1, p));
		emit labelsReordered(tq(column->name()));
	}
}

void DataSetPackage::labelReverse(size_t colIdx)
{
	Column		*	column	= _dataSet->columns()[colIdx];

	column->labelsReverse();

	QModelIndex p = indexForSubNode(column);

	emit dataChanged(index(0, 0, p), index(rowCount(p) - 1, columnCount(p) - 1, p));
	emit labelsReordered(tq(column->name()));
}

void DataSetPackage::columnSetDefaultValues(const std::string & columnName, columnType columnType, bool emitSignals)
{
	if(!_dataSet)
		return;

	int colIndex = getColumnIndex(columnName);

	if(colIndex >= 0)
	{
		Column * column = _dataSet->columns()[colIndex];
		column->setDefaultValues(columnType);

		QModelIndex p = indexForSubNode(_dataSet->dataNode());


		if(emitSignals)
		{
			emit dataChanged(index(0, colIndex, p), index(rowCount(p), colIndex, p));
			emit headerDataChanged(Qt::Horizontal, colIndex, colIndex);
		}
	}
}

std::string DataSetPackage::freeNewColumnName(size_t startHere)
{
	const QString nameBase = tr("Column %1");

	while(true)
	{
		const std::string & newColName = fq(nameBase.arg(++startHere));
		if(isColumnNameFree(newColName))
			return newColName;
	}
}

Json::Value DataSetPackage::serializeColumn(const std::string& columnName) const
{
	Column*	column	= _dataSet->column(columnName);
	return column ? column->serialize() : Json::nullValue;
}

void DataSetPackage::deserializeColumn(const std::string& columnName, const Json::Value& col)
{
	Column		*	column	= _dataSet->column(columnName);
	column->deserialize(col);
	emit datasetChanged({tq(columnName)}, {}, {}, false, false);
}

const stringset& DataSetPackage::workspaceEmptyValues() const
{
	static stringset emptyVec;
	return _dataSet ? _dataSet->workspaceEmptyValues() : emptyVec;
}
}

bool DataSetPackage::workspaceShowRSyntax() const
{
	return workspace() ? workspace()->showRSyntax() : PreferencesModel::prefs()->showRSyntaxInResults();
}


void DataSetPackage::setDataSetEmptyValues(const stringset &emptyValues, bool reset)
{
	if (!workspace()) 
		return;
	
	
	for(DataSet * dataSet : workspace()->dataSets())
		dataSet->setEmptyValuesFromStrings(emptyValues);
	
	if(reset)	
		refresh();
	
	emit workspaceEmptyValuesChanged();
}

void DataSetPackage::setDefaultWorkspaceEmptyValues()
{
	stringvec prefs = fq(PreferencesModel::prefs()->emptyValues());
	setDataSetEmptyValues(stringset(prefs.begin(), prefs.end()));
}

void DataSetPackage::setWorkspaceShowRSyntax(bool show)
{
	if (!workspace() || workspace()->showRSyntax() == show) 
		return;

	workspace()->setShowRSyntax(show);

	setModified(true);
}


void DataSetPackage::setCurrentFile(QString currentFile)
{
	if (_currentFile == currentFile)
		return;

	_currentFile = currentFile;
	emit currentFileChanged();

	QFileInfo	file(_currentFile);
	QUrl		url(_currentFile);

#ifdef _WIN32
	setFolder(file.exists() ? file.absolutePath().replace('/', '\\')	: url.isValid() ? "OSF" : "");
#else
	setFolder(file.exists() ? file.absolutePath()						: url.isValid() ? "OSF" : "");
#endif
}

void DataSetPackage::setFolder(QString folder)
{
	//Remove the last part if it is the name of the file regardless of extension
	QString _name	= name();
	int		i		= _name.size();
	for(; i < folder.size(); i++)
		if(folder.right(i).startsWith(_name))
		{
			folder = folder.left(folder.size() - i);
			break;
		}
#ifdef _WIN32
		else if(folder.right(i).contains('\\'))	break;
#else
		else if(folder.right(i).contains('/'))	break;
#endif

	if (_folder == folder)
		return;

	_folder = folder;
	emit folderChanged();
}

QString DataSetPackage::name() const
{
	QFileInfo	file(_currentFile);

	if(file.completeBaseName() != "")
		return file.completeBaseName();

	return "JASP";
}

bool DataSetPackage::dataMode() const
{
	return workspace() && workspace()->dataMode();
}

QString DataSetPackage::windowTitle() const
{
	QString name	= DataSetPackage::name(),
			folder	= DataSetPackage::folder();
	
#ifdef _WIN32
	if(folder.startsWith(AppDirs::examples().replace('/', '\\')))
#else
	if(folder.startsWith(AppDirs::examples()))
#endif
		folder = "";

	folder = folder == "" ? "" : "      (" + folder + ")";

	return name + (isModified() ? isModifiedAfterAutoSave() ? "*" : "* (autosaved)"  : "") + folder;
}


bool DataSetPackage::currentJaspFileIsNonSaveable() const
{
	return filePathIsNonSaveable(currentFile());
}

bool DataSetPackage::filePathIsNonSaveable(const QString & path) const
{
	QFileInfo fileDir(path);

	return fileDir.dir().absolutePath().startsWith(AppDirs::examples()) || fileDir.dir() == QDir(AppDirs::autoSaveDir());
}

void DataSetPackage::setAnalysesData(const Json::Value &analysesData)
{
	QString		previousASF					= analysesData.type() != Json::objectValue ? "" : tq(analysesData.get("autoSaveFileName", "").asString());
				_analysesData				= analysesData;
	QFileInfo	dataFile					( tq(dataSet() ? dataSet()->dataFilePath() : "") ),
				curFileI					( currentFile() );
	QString		dataFileName				= dataFile.fileName(),
				curFile						= currentFile(),
				autoSaveString				= curFile != "JASP" ? tr("%1 autosaved").arg(curFileI.fileName()) + "<br>" + tr("Full path: %1").arg("<code>"+curFileI.absoluteFilePath()+"</code>") : dataFileName == "" ? tr("Unsaved workspace") : tr("Unsaved workspace of datafile %1").arg(dataFileName);

	_analysesData["autoSaveDescription"]	= fq(autoSaveString);
	_analysesData["autoSaveFileName"]		= fq(curFileI.exists() ? curFileI.fileName() : previousASF != "" ? previousASF : curFile != "" ? curFile : tr("Autosave"));
}


QString DataSetPackage::autoSavedFileName() const
{
	return tq(_analysesData.get("autoSaveFileName", fq(currentFile())).asString());
}

// This function can be called from a different thread then where the underlying value for isReady() is set, but I don't think a mutex or whatever is necessary here. What could go wrong with checking a boolean?
// Also this was already the case, so I'm not making things worse here...
void DataSetPackage::waitForExportResultsReady() 
{ 
	int maxSleepTime	= 10000,
		sleepTime		= 100,
		delay			= 0;
	
	while (!isReady())
	{
		if (delay > maxSleepTime)
			break;
		
		Utils::sleep(sleepTime);
		delay += sleepTime;
	}
	
	if(!isReady())
		Log::log() << "Results were not exported properly!" << std::endl; //Should we maybe create a dummy result that explains something went wrong with the upload? Should we abort saving? What is going on?
}


void DataSetPackage::checkDataSetForUpdates()
{
	if(!_workspace)
		return;

	_workspace->checkForUpdates();
}

bool DataSetPackage::manualEdits() const
{
	return _manualEdits;
}

void DataSetPackage::setManualEdits(bool newManualEdits)
{
	if (_manualEdits == newManualEdits)
		return;

	_manualEdits = newManualEdits;

	emit manualEditsChanged();
}

