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
#include "utilities/qutils.h"
#include <QThread>
#include "engine/enginesync.h"
#include "columnencoder.h"
#include "timers.h"
#include "utilities/appdirs.h"
#include "utils.h"
#include "columnutils.h"
#include "utilities/messageforwarder.h"
#include "databaseconnectioninfo.h"
#include "utilities/settings.h"
#include "modules/ribbonmodel.h"
#include "filtermodel.h"


DataSetPackage * DataSetPackage::_singleton = nullptr;

DataSetPackage::DataSetPackage(QObject * parent) : QObject(parent)
{
	if(_singleton) throw std::runtime_error("DataSetPackage can be constructed only once!");
	_singleton = this;
	//True init is done in setEngineSync!
	
	_db			= new DatabaseInterface(true);

	_dataSet	= new DataSetQ(); //We create one here to make sure filter() etc can actually work
	setDefaultWorkspaceEmptyValues();
	
	connect(this, &DataSetPackage::isModifiedChanged,	this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::loadedChanged,		this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::currentFileChanged,	this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::folderChanged,		this, &DataSetPackage::windowTitleChanged);
	connect(this, &DataSetPackage::currentFileChanged,	this, &DataSetPackage::nameChanged);
	connect(this, &DataSetPackage::dataModeChanged,		this, &DataSetPackage::onDataModeChanged);

	connect(&_databaseIntervalSyncher,	&QTimer::timeout, this, &DataSetPackage::synchingIntervalPassed);

	new UndoStack(this);
}

DataSetPackage::~DataSetPackage() 
{ 
	_databaseIntervalSyncher.stop();

	_singleton = nullptr; 
}


void DataSetPackage::createDataSet()
{
	JASPTIMER_SCOPE(DataSetPackage::createDataSet);
					
	deleteDataSet();
	_dataSet = new DataSetQ();
	connectDataSet();
	setDefaultWorkspaceEmptyValues();
	
	_dataSet->shownFilter()->setRFilter(FilterModel::defaultRFilter());
	
	 
}

void DataSetPackage::loadDataSet(std::function<void(float)> progressCallback)
{
	if(_dataSet)
		deleteDataSet(); //no dbDelete necessary cause we just copied an old sqlite file here from the JASP file
	
	_db->close();
	_db->load();		
	_db->upgradeDBFromVersion(_jaspVersion);
	
	bool do019Upgrade = _jaspVersion < "0.19"; // A tweak needs to be made to the data as its loaded, see https://github.com/jasp-stats/jasp-desktop/pull/5367
	
	_dataSet = new DataSetQ(0);
	connectDataSet();
	_dataSet->dbLoad(1, progressCallback, do019Upgrade); //Right now there can only be a dataSet with ID==1 so lets keep it simple
	if (do019Upgrade)
	{
		// In 0.18.3 and before, there was a bug with the order of dataFilePath and description in the database.
		// dataFilePath was set empty and description has dataFilePath.
		if (dataFilePath().empty())
		{
			QFileInfo fileInfo(description());
			if (fileInfo.isFile())
				setDataFilePath(fq(description()));
		}
	}

	DataSetPackage::pkg()->initializeComputedColumns();

	emit synchingExternallyChanged(synchingExternally());
}

void DataSetPackage::deleteDataSet()
{
	JASPTIMER_SCOPE(DataSetPackage::deleteDataSet);
	
	dbDelete();
	delete _dataSet;
	_dataSet = nullptr;
	_undoStack->clear();
	
	emit dataSetQChanged();
}

void DataSetPackage::connectDataSet()
{
	if(!_dataSet)
		return;
	
	connect(_dataSet, &DataSetQ::runFilter,				this, &DataSetPackage::runFilter			);
	connect(_dataSet, &DataSetQ::showWarning,			this, &DataSetPackage::showWarning			);
	connect(_dataSet, &DataSetQ::manualEditMade,		this, &DataSetPackage::setManualEdits		);
	connect(_dataSet, &DataSetQ::datasetChanged,		this, &DataSetPackage::datasetChanged		);
	connect(_dataSet, &DataSetQ::labelsReordered,		this, &DataSetPackage::labelsReordered		);
	connect(_dataSet, &DataSetQ::columnTypeChanged,		this, &DataSetPackage::columnDataTypeChanged);
	connect(_dataSet, &DataSetQ::columnsBeingRemoved,	this, &DataSetPackage::columnsBeingRemoved	);
	connect(_dataSet, &DataSetQ::labelFilterChanged,	this, &DataSetPackage::labelFilterChanged	);
	connect(_dataSet, &DataSetQ::somethingModified,		this, &DataSetPackage::setModified			);
	
	emit dataSetQChanged();
}


Filter * DataSetPackage::filter()
{
    return !pkg()->_dataSet ? nullptr : pkg()->_dataSet->shownFilter();
}

void DataSetPackage::setEngineSync(EngineSync * engineSync)
{
	_engineSync = engineSync;

	//These signals should *ONLY* be called from a different thread than _engineSync!
	connect(this,	&DataSetPackage::enginesReceiveNewDataSignal,	_engineSync,	&EngineSync::enginesReceiveNewData,	Qt::BlockingQueuedConnection);

	reset();
}

bool DataSetPackage::isThisTheSameThreadAsEngineSync()
{
	return	QThread::currentThread() == _engineSync->thread();
}

void DataSetPackage::enginesReceiveNewData()
{
	if(!dataMode())
	{
		if(isThisTheSameThreadAsEngineSync())	_engineSync->enginesReceiveNewData();
		else									emit enginesReceiveNewDataSignal();
	}

	ColumnEncoder::setCurrentColumnNames(getColumnNames()); //Same place as in engine, should be fine right?
}

void DataSetPackage::reset(bool newDataSet)
{
	Log::log() << "DataSetPackage::reset()" << std::endl;
	_databaseIntervalSyncher.stop();
	_delayedRefreshTimer.stop();
	
	beginLoadingData();

	if(newDataSet)	createDataSet();
	else			deleteDataSet();

	_archiveVersion				= Version();
	_jaspVersion				= Version();
	_analysesHTML				= QString();
	_analysesData				= Json::arrayValue;
	_warningMessage				= std::string();
	_hasAnalysesWithoutData		= false;
	_analysesHTMLReady			= false;
	_database					= Json::nullValue;
	_isJaspFile					= false;
	_filterShouldRunInit		= false;
	_manualEdits				= false;

	setLoaded(false);
	setModified(false);
	setSynchingExternally(false); //Default is off, AsyncLoader::loadPackage(...) will turn it on for non-jasp
	setCurrentFile("");

	endLoadingData();
}

void DataSetPackage::generateEmptyData()
{
	if(isLoaded())
	{
		Log::log() << "void DataSetPackage::generateEmptyData() called but dataset already loaded, ignoring it." << std::endl;
		return;
	}

	const int INIT_COL = 1;
	const int INIT_ROW = 1;

	beginLoadingData();

	if(!_dataSet)
		createDataSet();
	setDataSetSize(INIT_COL, INIT_ROW);
	doublevec emptyValues(INIT_ROW, EmptyValues::missingValueDouble);
	initColumnWithStrings(0, freeNewColumnName(0), {""});

	endLoadingData();
	emit newDataLoaded();
	resetAllFilters();
	setSynchingExternally(false);
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

void DataSetPackage::setModified(bool value)
{
	if ((!value || _isLoaded || _hasAnalysesWithoutData) && value != _isModified)
	{
		_isModified = value;
		emit isModifiedChanged();
	}
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
	return tq(_dataSet ? _dataSet->description() : "");
}

bool DataSetPackage::dataFileCanHaveLabels() const
{ 
	return _dataSet && !tq(_dataSet->dataFilePath()).endsWith(".csv");  
}

void DataSetPackage::setDescription(const QString &description)
{
	if (!_dataSet) return;
	
	_dataSet->setDescription(fq(description));

	emit descriptionChanged();
}

int DataSetPackage::findIndexByName(const std::string & name) const
{
	return _dataSet->getColumnIndex(name);
}

bool DataSetPackage::isColumnNameFree(const std::string & name) const
{
	return findIndexByName(name) == -1;
}

bool DataSetPackage::isColumnComputed(size_t colIndex) const
{
	const Column * normalCol = _dataSet->columns().at(colIndex);
	
	return normalCol->isComputed();
}

bool DataSetPackage::isColumnComputed(const std::string & name) const
{
	const Column * normalCol = _dataSet->column(name);

	return normalCol && normalCol->isComputed();
}

bool DataSetPackage::isColumnAnalysisNotComputed(const std::string & name) const
{
	const Column * normalCol = _dataSet->column(name);

	return normalCol && normalCol->codeType() == computedColumnType::analysisNotComputed;
}

bool DataSetPackage::isColumnInvalidated(size_t colIndex) const
{
	return colIndex <= dataColumnCount() && _dataSet->columns().at(colIndex)->invalidated();
}

std::string DataSetPackage::getComputedColumnError(size_t colIndex) const
{
	return colIndex >= dataColumnCount() ? "" : _dataSet->columns().at(colIndex)->error();
}

QVariant DataSetPackage::getColumnTypesWithIcons() const
{
	static QVariantList ColumnTypeAndIcons;

	if(ColumnTypeAndIcons.size() == 0)
	{
		ColumnTypeAndIcons.push_back("");
		ColumnTypeAndIcons.push_back("variable-scale.svg");
		ColumnTypeAndIcons.push_back("variable-ordinal.svg");
		ColumnTypeAndIcons.push_back("variable-nominal.svg");
	}

	return QVariant(ColumnTypeAndIcons);
}




void DataSetPackage::columnWasOverwritten(const std::string & columnName, const std::string &)
{
	for(size_t col=0; col<_dataSet->columns().size(); col++)
		if(_dataSet->columns()[col]->name() == columnName)
			emit dataChanged(index(0, col, indexForSubNode(_dataSet->dataNode())), index(rowCount()-1, col, indexForSubNode(_dataSet->dataNode())) );
}

void DataSetPackage::beginSynchingData(bool informEngines)
{
	beginLoadingData(informEngines);
	_synchingData = true;
}

void DataSetPackage::endSynchingDataChangedColumns(stringvec &	changedColumns, bool hasNewColumns, bool informEngines)
{
	 stringvec				missingColumns;
	 strstrmap		changeNameColumns;

	endSynchingData(changedColumns, missingColumns, changeNameColumns, hasNewColumns, informEngines);
}

void DataSetPackage::endSynchingData(	const stringvec	&	changedColumns,
										const stringvec	&	missingColumns,
										const strstrmap	&	changeNameColumns,  //origname -> newname
										bool				rowCountChanged,
										bool				hasNewColumns,
										bool				informEngines)
{

	endLoadingData(informEngines);
	_synchingData = false;
	//We convert all of this stuff to qt containers even though this takes time etc. Because it needs to go through a (queued) connection and it might not work otherwise
	emit datasetChanged(tq(changedColumns), tq(missingColumns), tq(changeNameColumns), rowCountChanged, hasNewColumns);

	setManualEdits(false);
}

void DataSetPackage::endLoadingData(bool informEngines)
{
	JASPTIMER_SCOPE(DataSetPackage::endLoadingData);

	Log::log() << "DataSetPackage::endLoadingData" << std::endl;

	endResetModel();
	enginesReceiveNewData();

	emit modelInit();
}

void DataSetPackage::setDataSetSize(size_t columnCount, size_t rowCount)
{
	
	JASPTIMER_SCOPE(DataSetPackage::setDataSetSize);
		
	_dataSet->setColumnCount(columnCount);
	_dataSet->setRowCount(rowCount);
}

void DataSetPackage::dbDelete()
{
	JASPTIMER_SCOPE(DataSetPackage::dbDelete);
	if(_dataSet && _dataSet->id() != -1)
		_dataSet->dbDelete();
}

void DataSetPackage::resetVariableTypes()
{
	if(_dataSet)
		_dataSet->resetVariableTypes();
}

bool DataSetPackage::initColumnWithStrings(QVariant colID, const std::string & newName, const stringvec &values, const stringvec & labels, const std::string & title, columnType desiredType, const stringset & emptyValues)
{
	JASPTIMER_SCOPE(DataSetPackage::initColumnWithStrings);

	int colIndex	= colID.typeId() == QMetaType::Int || colID.typeId() == QMetaType::UInt
					? (colID.typeId() == QMetaType::Int ? colID.toInt() : colID.toUInt())
					: _dataSet->getColumnIndex(fq(colID.toString()));
	
	return _dataSet->initColumnWithStrings(
				colIndex, newName, values, labels, title, desiredType, emptyValues,
				Settings::value(Settings::THRESHOLD_SCALE).toInt(),
				PreferencesModel::prefs()->orderByValueByDefault());
}

void DataSetPackage::initializeComputedColumns()
{
	for(const Column * col : dataSet()->columns())
		emit checkForDependentColumnsToBeSent(tq(col->name()));
}


stringvec DataSetPackage::getColumnNames()
{
	if(_dataSet)
		return _dataSet->getColumnNames();
	return {};
}

bool DataSetPackage::isColumnDifferentFromStringValues(const std::string & columnName, const std::string & title, const stringvec & strVals, const stringvec & strLabs, const stringset & strEmptyVals)
{
	Column * col = _dataSet->column(columnName);
	
	if(col)
		return col->isColumnDifferentFromStringValues(title, strVals, strLabs, strEmptyVals);

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

	//emit dataChanged(index(0, columnIndex), index(rowCount() - 1, columnIndex));
	//we need to actually send lots of signals from ColumnModel but because of the undo/redo this is a bit convoluted now...

	refresh();
}

void DataSetPackage::setColumnComputedType(const std::string & columnName, computedColumnType type)
{
	setColumnComputedType(getColumnIndex(columnName), type);
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

QList<QVariant> DataSetPackage::getColumnValuesAsDoubleList(size_t columnIndex)	const
{
	QList<QVariant> list;
	if(columnIndex < 0 || columnIndex >= dataColumnCount()) return list;

	for (double value : _dataSet->columns()[columnIndex]->dbls())
		list.append(value);

	return list;
}


void DataSetPackage::labelMoveRows(size_t colIdx, std::vector<qsizetype> rows, bool up)
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

void DataSetPackage::setDefaultWorkspaceEmptyValues()
{
	stringvec prefs = fq(PreferencesModel::prefs()->emptyValues());
	setWorkspaceEmptyValues(stringset(prefs.begin(), prefs.end()));
}

void DataSetPackage::setWorkspaceEmptyValues(const stringset &emptyValues, bool reset)
{
	if (!_dataSet) return;
	
	if(reset)	beginResetModel();
	_dataSet->setWorkspaceEmptyValues(emptyValues);
	if(reset)	endResetModel();
	
	emit workspaceEmptyValuesChanged();
}

void DataSetPackage::pasteSpreadsheet(size_t row, size_t col, const std::vector<std::vector<QString>> & values, const std::vector<std::vector<QString>> &  labels, const intvec & coltypes, const QStringList & colNames, const std::vector<boolvec> & selected)
{
	JASPTIMER_SCOPE(DataSetPackage::pasteSpreadsheet);

	int		rowMax			= ( values.size() > 0 ? values[0].size() : 0), 
			colMax			= values.size();
	bool	rowCountChanged = int(row + rowMax) > dataRowCount()	,
			colCountChanged = int(col + colMax) > dataColumnCount()	;
	
	auto isSelected = [&selected](int row, int col)
	{
		return selected.size() == 0 || 	selected[col][row];
	};

	_dataSet->beginBatchedToDB();
	
	if(colCountChanged || rowCountChanged)	
		setDataSetSize(std::max(size_t(dataColumnCount()), colMax + col), std::max(size_t(dataRowCount()), rowMax + row));
	
	stringvec changed;
	strstrmap changeNameColumns;

	for(int c=0; c<colMax; c++)
	{
		Column	*	column		= _dataSet->column(c + col);
		columnType	desiredType	= coltypes.size() > c ? columnType(coltypes[c]) : column->type();
					desiredType = desiredType == columnType::unknown ? columnType::scale : desiredType;
		std::string colName		= (colNames.size() > c && !colNames[c].isEmpty()) ? fq(colNames[c]) : column->name();
		
		column->setType(desiredType);

		bool aChange = false;
		for(int r=0; r<rowMax; r++)
			if(isSelected(r, c))
				aChange = column->setStringValue(r+row, fq(values[c][r]), labels.size() <= c || labels[c].size() <= r ? "" : fq(labels[c][r])) || aChange;
			
		aChange = aChange || colName != column->name() || desiredType != column->type();
		
		if(colName != column->name())
			changeNameColumns[column->name()] = colName;
		
		column->setName(colName);

		if(aChange)
		{
			changed.push_back(colName);
			column->labelsTempReset();
		}
	}

	_dataSet->endBatchedToDB();
	
	stringvec		missingColumns;

	emit datasetChanged(tq(changed), tq(missingColumns), tq(changeNameColumns), rowCountChanged, colCountChanged);
}

QString DataSetPackage::insertColumnSpecial(int columnIndex, const QMap<QString, QVariant>& props)
{
	if(columnIndex < 0)
		columnIndex = 0;

	if(columnIndex > dataColumnCount())
		columnIndex = dataColumnCount(); //the column will be created if necessary but only if it is in a logical place. So the end of the vector

	setManualEdits(true); //Don't synch with external file after editing
#ifdef ROUGH_RESET
	beginResetModel();
#else
	beginInsertColumns(indexForSubNode(_dataSet->dataNode()), columnIndex, columnIndex);
#endif

	_dataSet->insertColumn(columnIndex);
	
	Column * column = _dataSet->column(columnIndex);

	column->setName(			props.contains("name")		? fq(props["name"].toString())					: freeNewColumnName(columnIndex)	);
	column->setDefaultValues(	props.contains("type")		? columnType(props["type"].toInt())				: columnType::scale					);
	column->setCodeType(		props.contains("computed")	? computedColumnType(props["computed"].toInt())	: computedColumnType::notComputed	);

	_dataSet->incRevision();

#ifdef ROUGH_RESET
	endResetModel();
#else
	endInsertColumns();
#endif
	
	emit datasetChanged(tq(stringvec{column->name()}), {}, {}, false, true);

	ColumnEncoder::setCurrentColumnNames(getColumnNames());
	
	if(column->codeType() == computedColumnType::constructorCode || column->codeType() == computedColumnType::rCode)
		emit columnAddedManually(tq(column->name())); //Will trigger setChosenColumn and setVisible(true) on ColumnModel, showing it to the user

	return QString::fromStdString(column->name());
}

QString DataSetPackage::appendColumnSpecial(const QMap<QString, QVariant>& props)
{
	return insertColumnSpecial(dataColumnCount(), props);
}


bool DataSetPackage::insertColumns(int column, int count, const QModelIndex & aparent)
{

}

bool DataSetPackage::removeColumns(int column, int count, const QModelIndex & aparent)
{
	
}

bool DataSetPackage::insertRows(int row, int count, const QModelIndex & aparent)
{
	
}

bool DataSetPackage::removeRows(int row, int count, const QModelIndex & aparent)
{
	
}

Column * DataSetPackage::createColumn(const std::string & name, columnType columnType)
{
	if(getColumnIndex(name) >= 0)
		return nullptr;

	setModified(true);

	size_t newColumnIndex	= dataColumnCount();

	enginesPrepareForData();
	beginResetModel();
	_dataSet->insertColumn(newColumnIndex);
	_dataSet->column(newColumnIndex)->setName(name);
	_dataSet->column(newColumnIndex)->setDefaultValues(columnType);
	endResetModel();
	enginesReceiveNewData();

	return _dataSet->column(newColumnIndex);
}

void DataSetPackage::removeColumn(const std::string & name)
{
	int colIndex = getColumnIndex(name);
	if(colIndex == -1) return;

	removeColumns(colIndex, 1);
}

bool DataSetPackage::columnExists(Column *column)
{
	if(_dataSet)
		for(const Column * col : _dataSet->columns())
			if(col == column)
				return true;
		
	return false;
}

void DataSetPackage::columnsReorder(const stringvec &order)
{
	beginResetModel();
	_dataSet->columnsReorder(order);
	endResetModel();
}

boolvec DataSetPackage::filterVector()
{
	boolvec out;

	if(_dataSet)
            out = boolvec(_dataSet->shownFilter()->filtered().begin(), _dataSet->shownFilter()->filtered().end());
	
	return out;
}

void DataSetPackage::databaseStopSynching()
{
	_databaseIntervalSyncher.stop();	
}

void DataSetPackage::databaseStartSynching(bool syncImmediately)
{
	if(_database == Json::nullValue)
		throw std::runtime_error("Cannot start synching with a database if we arent connected to a database...");
	
	_databaseIntervalSyncher.stop(); //Is this even necessary? Probaly not but lets do it just in case

	DatabaseConnectionInfo dbCI(_database);
	
	if(dbCI._interval > 0)
	{
		if(dbCI._hadPassword && !dbCI._rememberMe && dbCI._password == "")
		{
			bool tryAgain = true, couldConnect;

			while(tryAgain)
			{
				dbCI._password	= MessageForwarder::askPassword(tr("Database Password"), dbCI._username != "" ? tr("The databaseconnection needs a password for user '%1'").arg(dbCI._username) : tr("The databaseconnection needs a password"));
				tryAgain		= dbCI.connect() ? false : MessageForwarder::showYesNo(tr("Connection failed"), tr("Could not connect to database because of '%1', want to try a different password?").arg(dbCI.lastError()));
			}

			if(!dbCI.connected())
			{
				MessageForwarder::showWarning(tr("Could not connect to the database so synchronizing will be disabled."));
				return;
			}
			else
				_database = dbCI.toJson();
		}
	
		_databaseIntervalSyncher.setInterval(1000 * 60 * dbCI._interval);
		_databaseIntervalSyncher.start();
		
		if(syncImmediately)
			emit synchingIntervalPassed();

		emit synchingExternallyChanged(synchingExternally());
	}
}

bool DataSetPackage::synchingExternally() const
{
	return _dataSet && _dataSet->dataFileSynch() && (!_dataSet->dataFilePath().empty() || (_database != Json::nullValue && _databaseIntervalSyncher.isActive()));
}

void DataSetPackage::setSynchingExternallyFriendly(bool synchingExternally)
{
	if (synchingExternally && emit askUserForExternalDataFile())
	{
		setSynchingExternally(synchingExternally);
		setModified(true); //Perhaps someone would like to save the fact that it shouldnt be synchronized
	}
	else if(!synchingExternally)
	{
		if(_dataSet && _dataSet->dataFileSynch())
			setSynchingExternally(false);
		setModified(true);
	}
}

void DataSetPackage::setSynchingExternally(bool synchingExternally)
{	
	if(_dataSet)
		_dataSet->setDataFileSynch(synchingExternally);

	emit synchingExternallyChanged(DataSetPackage::synchingExternally());
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
	return RibbonModel::singleton()->dataMode();
}

QModelIndex DataSetPackage::lastCurrentCell()
{
 throw std::runtime_error("Not implemented!");
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

	return name + (isModified() ? "*" : "") + folder;
}

bool DataSetPackage::currentFileIsExample() const
{
	return currentFile().startsWith(AppDirs::examples());
}

void DataSetPackage::setDataFilePath(std::string filePath, long timestamp)
{
	if(!_dataSet || (_dataSet->dataFilePath() == filePath && timestamp == _dataSet->dataFileTimestamp()))
		return;

	if (timestamp == 0 && !filePath.empty())
	{
		QFileInfo fileInfo(tq(filePath));
		timestamp = fileInfo.isFile() ? fileInfo.lastModified().toSecsSinceEpoch() : 0;
	}

	_dataSet->setDataFile(filePath, timestamp);
	if (tq(filePath).startsWith(AppDirs::examples()))
		setDataFileReadOnly(true);

	setModified(true);
	emit synchingExternallyChanged(synchingExternally());
}

void DataSetPackage::setDatabaseJson(const Json::Value &dbInfo)		
{
	_database						= dbInfo;			
	Log::log() << "DataSetPackage::setDatabaseJson got:" << dbInfo << std::endl;

	_dataSet->setDatabaseJson(_database.toStyledString());
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


void DataSetPackage::checkComputedColumnDependenciesForAnalysis(Analysis * analysis)
{
	if(!_dataSet)
		return;

	for(Column * col : _dataSet->columns())
		if(col->isComputedByAnalysis(analysis->id()))
			col->setDependsOn(analysis->usedVariables());

}

stringset DataSetPackage::columnsCreatedByAnalysis(Analysis * analysis)
{
	if(!_dataSet)
		return {};

	stringset cols;

	for(Column * col : _dataSet->columns())
		if(col->analysisId() == analysis->id())
			cols.insert(col->name());

	return cols;
}

Column * DataSetPackage::createComputedColumn(const std::string & name, columnType type, computedColumnType desiredType, Analysis * analysis)
{
	QString nameTemp = insertColumnSpecial(dataColumnCount(), { std::make_pair("computed", int(desiredType)) });

	Column	* newComputedColumn = DataSetPackage::pkg()->dataSet()->column(nameTemp.toStdString());

	beginResetModel();

	newComputedColumn->setName(name);
	newComputedColumn->setType(type);

	if(analysis)
		newComputedColumn->setAnalysisId(analysis->id());

	endResetModel();

	return newComputedColumn;
}

Column * DataSetPackage::requestComputedColumnCreation(const std::string & columnName, Analysis * analysis)
{
	if(!DataSetPackage::pkg()->isColumnNameFree(columnName))
		return nullptr;

	return createComputedColumn(columnName, columnType::scale, computedColumnType::analysis, analysis);
}

bool DataSetPackage::requestColumnCreation(const std::string & columnName, Analysis * analysis, columnType type)
{
	if(!DataSetPackage::pkg()->isColumnNameFree(columnName))
		return false;
	
	createComputedColumn(columnName, type, computedColumnType::analysisNotComputed, analysis);
	return true;
}


bool DataSetPackage::requestComputedColumnDestruction(const std::string& columnName, Analysis * analysis)
{
	if(columnName.empty())
		return false;

	Column * col = dataSet()->column(columnName);

	if(!col || !col->isComputed() || !col->isComputedByAnalysis(analysis->id()))
		return false;

	removeColumn(columnName);

	emit checkForDependentColumnsToBeSent(tq(columnName));
	
	return true;
}

void DataSetPackage::checkDataSetForUpdates()
{
	if(!_dataSet)
		return;

	stringvec changedCols, missingCols;
	bool newCols = false, rowCountChanged = false;

	if(_dataSet->checkForUpdates(&changedCols, &missingCols, &newCols, &rowCountChanged))
	{
		Log::log()	<< "Updates found for DataSet " << _dataSet->id() 
					<< "| missing cols: '" << tq(missingCols).join(",")
					<< "' | changed cols: '" << tq(changedCols).join(",")
					<< "' | " << (newCols ? " has new cols" : "") << (rowCountChanged ? "| rowcount changed |" : "|") << std::endl;
		refresh();

		emit datasetChanged(tq(changedCols), tq(missingCols), {}, newCols, rowCountChanged);
	}
}

bool DataSetPackage::manualEdits() const
{
	return _manualEdits;
}

void DataSetPackage::setManualEdits(bool newManualEdits)
{
	// During synchronization, even if some data are changed, manualEdits should not be set to true
	if ((_synchingData && newManualEdits) || _manualEdits == newManualEdits)
		return;

	_manualEdits = newManualEdits;

	if(_manualEdits)
		setSynchingExternally(false);

	emit manualEditsChanged();
}

