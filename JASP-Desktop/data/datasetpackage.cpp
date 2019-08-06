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

#include "log.h"
#include "utilities/qutils.h"
#include "sharedmemory.h"
#include <QThread>
#include "engine/enginesync.h"

#define ENUM_DECLARATION_CPP
#include "datasetpackage.h"

parIdxType DataSetPackage::_nodeCategory[] = { parIdxType::root, parIdxType::data, parIdxType::label, parIdxType::filter };	//Should be same order as defined

DataSetPackage::DataSetPackage(QObject * parent) : QAbstractItemModel(parent)
{
	//True init is done in setEngineSync!
}

void DataSetPackage::setEngineSync(EngineSync * engineSync)
{
	_engineSync = engineSync;

	//These signals should *ONLY* be called from a different thread than _engineSync!
	connect(this,	&DataSetPackage::pauseEnginesSignal,	_engineSync,	&EngineSync::pause,		Qt::BlockingQueuedConnection);
	connect(this,	&DataSetPackage::resumeEnginesSignal,	_engineSync,	&EngineSync::resume,	Qt::BlockingQueuedConnection);

	reset();
	informComputedColumnsOfPackage();
}

bool DataSetPackage::isThisTheSameThreadAsEngineSync()
{
	return	QThread::currentThread() == _engineSync->thread();
}

void DataSetPackage::pauseEngines()
{
	if(isThisTheSameThreadAsEngineSync())	_engineSync->pause();
	else									emit pauseEnginesSignal();
}

void DataSetPackage::resumeEngines()
{
	if(isThisTheSameThreadAsEngineSync())	_engineSync->resume();
	else									emit resumeEnginesSignal();
}

void DataSetPackage::reset()
{
	beginLoadingData();
	setDataSet(nullptr);
	_archiveVersion				= Version();
	_dataArchiveVersion			= Version();
	_analysesHTML				= std::string();
	_analysesData				= Json::arrayValue;
	_warningMessage				= std::string();
	_isLoaded					= false;
	_hasAnalysesWithoutData		= false;
	_analysesHTMLReady			= false;
	_isArchive					= false;
	_dataFilter					= DEFAULT_FILTER;
	_filterConstructorJSON		= DEFAULT_FILTER_JSON;
	_computedColumns			= ComputedColumns(this);

	setModified(false);
	resetEmptyValues();
	endLoadingData();
}

void DataSetPackage::setDataSet(DataSet * dataSet)
{
	_dataSet = dataSet;
}

void DataSetPackage::createDataSet()
{
	setDataSet(SharedMemory::createDataSet()); //Why would we do this here but the free in the asyncloader?
}

void DataSetPackage::freeDataSet()
{
	if(_dataSet)
		emit freeDatasetSignal(_dataSet);
	_dataSet = nullptr;
}

QModelIndex DataSetPackage::index(int row, int column, const QModelIndex &parent) const
{
	parIdxType * pointer = _nodeCategory;

	if(!parent.isValid())	pointer += row; //this index will be the actual data/labels/filter and will remember what type it is through the pointer
	else					pointer = static_cast<parIdxType*>(parent.internalPointer());

	return createIndex(row, column, static_cast<void*>(pointer));
}

parIdxType DataSetPackage::parentIndexTypeIs(const QModelIndex &index) const
{
	if(!index.isValid()) return parIdxType::root;

	parIdxType	* pointer = static_cast<parIdxType*>(index.internalPointer());
	return		* pointer; //Is defined in static array _nodeCategory!
}

QModelIndex DataSetPackage::parent(const QModelIndex & index) const
{
	parIdxType parentType = parentIndexTypeIs(index);

	return parentModelForType(parentType);
}



QModelIndex DataSetPackage::parentModelForType(parIdxType type, int column) const
{
	if(type == parIdxType::root) return QModelIndex();

	return index(int(type), column, QModelIndex());
}

int DataSetPackage::rowCount(const QModelIndex & parent) const
{
	switch(parentIndexTypeIs(parent))
	{
	case parIdxType::label:		return !_dataSet || _dataSet->columnCount() <= parent.column() ? 0 : _dataSet->columns()[parent.column()].labels().size();
	case parIdxType::filter:
	case parIdxType::root:		//return int(parIdxType::leaf); Its more logical to get the actual datasize
	case parIdxType::data:		return !_dataSet ? 0 : _dataSet->rowCount();
	}

	return 0; // <- because gcc is stupid
}

int DataSetPackage::columnCount(const QModelIndex &parent) const
{
	switch(parentIndexTypeIs(parent))
	{
	case parIdxType::filter:	return 1;
	case parIdxType::label:		return 3; //The parent index has a column index in it that tells you which actual column was selected!
	case parIdxType::root:		//Default is columnCount of data because it makes programming easier. I do hope it doesn't mess up the use of the tree-like-structure of the data though
	case parIdxType::data:		return _dataSet == nullptr ? 0 : _dataSet->columnCount();
	}

	return 0; // <- because gcc is stupid
}

bool DataSetPackage::getRowFilter(int row) const
{
	QModelIndex filterParent(parentModelForType(parIdxType::filter));

	return data(this->index(row, 0, filterParent)).toBool();
}

QVariant DataSetPackage::data(const QModelIndex &index, int role) const
{
	if(!index.isValid()) return QVariant();

	parIdxType parentType = parentIndexTypeIs(index);

	switch(parentType)
	{
	default:
		return QVariant();

	case parIdxType::filter:
		if(_dataSet == nullptr || index.row() < 0 || index.row() >= _dataSet->filterVector().size())
			return true;
		return _dataSet->filterVector()[index.row()];

	case parIdxType::data:
		if(_dataSet == nullptr || index.column() >= _dataSet->columnCount() || index.row() >= _dataSet->rowCount())
			return QVariant(); // if there is no data then it doesn't matter what role we play

		switch(role)
		{
		case Qt::DisplayRole:						return tq(_dataSet->column(index.column())[index.row()]);
		case int(specialRoles::value):				return tq(_dataSet->column(index.column()).getOriginalValue(index.row()));
		case int(specialRoles::filter):				return getRowFilter(index.row());
		case int(specialRoles::columnType):			return _dataSet->columns()[index.column()].columnType();
		case int(specialRoles::columnIsFiltered):	return _dataSet->columns()[index.column()].hasFilter();
		case int(specialRoles::lines):
		{
			bool	iAmActive		= getRowFilter(index.row()),
					belowMeIsActive = index.row() < rowCount() - 1	&& data(this->index(index.row() + 1, index.column(), index.parent()), int(specialRoles::filter)).toBool();


			bool	up		= iAmActive,
					left	= iAmActive,
					down	= iAmActive && !belowMeIsActive,
					right	= iAmActive && index.column() == columnCount(index.parent()) - 1; //always draw left line and right line only if last col

			return			(left ?		1 : 0) +
							(right ?	2 : 0) +
							(up ?		4 : 0) +
							(down ?		8 : 0);
		}
		}

	case parIdxType::label:
	{
		int parColCount = columnCount(index.parent()),
			parRowCount = rowCount(index.parent());

		if(!_dataSet || index.column() >= parColCount || index.row() >= parRowCount)
			return QVariant(); // if there is no data then it doesn't matter what role we play

		//We know which column we need through the parent index!
		Labels & labels = _dataSet->column(index.parent().column()).labels();

		switch(role)
		{
		case int(specialRoles::filter):				return labels[index.row()].filterAllows();
		case int(specialRoles::value):				return tq(labels.getValueFromRow(index.row()));
		case Qt::DisplayRole:
		default:									return tq(labels.getLabelFromRow(index.row()));
		}
	}
	}

	return QVariant(); // <- because gcc is stupid
}

size_t DataSetPackage::getMaximumColumnWidthInCharacters(int columnIndex) const
{
	return _dataSet ? _dataSet->getMaximumColumnWidthInCharacters(columnIndex) : 0;
}

QVariant DataSetPackage::headerData(int section, Qt::Orientation orientation, int role)	const
{
	if (_dataSet == nullptr)
		return QVariant();

	switch(role)
	{
	case int(specialRoles::maxColString):
	{
		//calculate some kind of maximum string to give views an expectation of the width needed for a column
		QString dummyText = headerData(section, orientation, Qt::DisplayRole).toString() + "XXXXX" + (isColumnComputed(section) ? "XXXXX" : ""); //Bit of padding for filtersymbol and columnIcon
		int colWidth = getMaximumColumnWidthInCharacters(section);

		while(colWidth > dummyText.length())
			dummyText += "X";

		return dummyText;
	}
	case Qt::DisplayRole:									return orientation == Qt::Horizontal ? tq(_dataSet->column(section).name()) : QVariant(section + 1);
	case Qt::TextAlignmentRole:								return QVariant(Qt::AlignCenter);
	case int(specialRoles::columnIsComputed):				return isColumnComputed(section);
	case int(specialRoles::computedColumnIsInvalidated):	return isColumnInvalidated(section);
	case int(specialRoles::columnIsFiltered):				return columnHasFilter(section) || columnUsedInEasyFilter(section);
	case int(specialRoles::labelsHasFilter):				return columnHasFilter(section);
	case int(specialRoles::computedColumnError):			return tq(getComputedColumnError(section));
	}

	return QVariant();
}

bool DataSetPackage::setData(const QModelIndex &index, const QVariant &value, int role)
{
	throw std::runtime_error("DataSetPackage::setData is not implemented yet!");

	/*if (_dataSet == nullptr)
		return false;

	bool ok;

	Column &column = _dataSet->columns()[index.column()];
	if (column.dataType() == Column::DataTypeInt)
	{
		int v = value.toInt(&ok);
		if (ok)
			column.setValue(index.row(), v);
		else
			emit badDataEntered(index);

		return ok;
	}*/

	//_dataSet->columns()[index.column()].setValue(index.row(), v);

}

Qt::ItemFlags DataSetPackage::flags(const QModelIndex &index) const
{
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QHash<int, QByteArray> DataSetPackage::roleNames() const
{
	static bool						set = false;
	static QHash<int, QByteArray> roles = QAbstractItemModel::roleNames ();

	if(!set)
	{
		roles[int(specialRoles::value)]							= QString("value").toUtf8();
		roles[int(specialRoles::lines)]							= QString("lines").toUtf8();
		roles[int(specialRoles::filter)]						= QString("filter").toUtf8();
		roles[int(specialRoles::columnType)]					= QString("columnType").toUtf8();
		roles[int(specialRoles::maxColString)]					= QString("maxColString").toUtf8();
		roles[int(specialRoles::labelsHasFilter)]				= QString("labelsHasFilter").toUtf8();
		roles[int(specialRoles::columnIsComputed)]				= QString("columnIsComputed").toUtf8();
		roles[int(specialRoles::computedColumnError)]			= QString("computedColumnError").toUtf8();
		roles[int(specialRoles::computedColumnIsInvalidated)]	= QString("computedColumnIsInvalidated").toUtf8();

		set = true;
	}

	return roles;
}

void DataSetPackage::setModified(bool value)
{
	if ((!value || _isLoaded || _hasAnalysesWithoutData) && value != _isModified)
	{
		_isModified = value;
		emit isModifiedChanged(this);
	}
}


bool DataSetPackage::isColumnNameFree(std::string name) const
{
	try			{ _dataSet->columns().findIndexByName(name); return false;}
	catch(...)	{ }

	return true;
}

bool DataSetPackage::isColumnComputed(size_t colIndex) const
{
	try
	{
		const Column & normalCol = _dataSet->columns().at(colIndex);
		_computedColumns.findIndexByName(normalCol.name());
		return true;
	}
	catch(...) {}

	return false;

}

bool DataSetPackage::isColumnComputed(std::string name) const
{
	try
	{
		_computedColumns.findIndexByName(name);
		return true;
	}
	catch(...) {}

	return false;

}

bool DataSetPackage::isColumnInvalidated(size_t colIndex) const
{
	try
	{
		const Column & normalCol		= _dataSet->columns().at(colIndex);
		const ComputedColumn & compCol	= _computedColumns[normalCol.name()];

		return compCol.isInvalidated();
	}
	catch(...) {}

	return false;
}

std::string DataSetPackage::getComputedColumnError(size_t colIndex) const
{
	try
	{
		const Column & normalCol		= _dataSet->columns().at(colIndex);
		const ComputedColumn & compCol	= _computedColumns[normalCol.name()];

		return compCol.error();
	}
	catch(...) {}

	return "";
}


ComputedColumns	* DataSetPackage::computedColumnsPointer()
{
	return &_computedColumns;
}


void DataSetPackage::setColumnsUsedInEasyFilter(std::set<std::string> usedColumns)
{
	_columnNameUsedInEasyFilter.clear();

	for(auto & col : usedColumns)
	{
		_columnNameUsedInEasyFilter[col] = true;
		try { notifyColumnFilterStatusChanged(_dataSet->columns().findIndexByName(col)); } catch(...) {}
	}
}


bool DataSetPackage::columnUsedInEasyFilter(int column) const
{
	if(_dataSet != nullptr && size_t(column) < _dataSet->columnCount())
	{
		std::string colName = _dataSet->column(column).name();
		return _columnNameUsedInEasyFilter.count(colName) > 0 && _columnNameUsedInEasyFilter.at(colName);
	}
	return false;
}

void DataSetPackage::notifyColumnFilterStatusChanged(int columnIndex)
{
	emit columnsFilteredCountChanged();
	emit headerDataChanged(Qt::Horizontal, columnIndex, columnIndex);
}


QVariant DataSetPackage::getColumnTypesWithCorrespondingIcon() const
{
	static QVariantList ColumnTypeAndIcons;

	//enum ColumnType { ColumnTypeUnknown = 0, ColumnTypeNominal = 1, ColumnTypeNominalText = 2, ColumnTypeOrdinal = 4, ColumnTypeScale = 8 };

	if(ColumnTypeAndIcons.size() == 0)
	{
		ColumnTypeAndIcons.push_back(QVariant(QString("")));
		ColumnTypeAndIcons.push_back(QVariant(QString("qrc:///icons/variable-nominal.svg")));
		ColumnTypeAndIcons.push_back(QVariant(QString("qrc:///icons/variable-nominal-text.svg")));
		ColumnTypeAndIcons.push_back(QVariant(QString("")));
		ColumnTypeAndIcons.push_back(QVariant(QString("qrc:///icons/variable-ordinal.svg")));
		ColumnTypeAndIcons.push_back(QVariant(QString("")));
		ColumnTypeAndIcons.push_back(QVariant(QString("")));
		ColumnTypeAndIcons.push_back(QVariant(QString("")));
		ColumnTypeAndIcons.push_back(QVariant(QString("qrc:///icons/variable-scale.svg")));
	}

	return QVariant(ColumnTypeAndIcons);
}


QVariant DataSetPackage::columnTitle(int column) const
{
	if(_dataSet != nullptr && column >= 0 && size_t(column) < _dataSet->columnCount())
		return tq(_dataSet->column(column).name());

	return QVariant();
}

QVariant DataSetPackage::columnIcon(int column) const
{
	if(_dataSet != nullptr && column >= 0 && size_t(column) < _dataSet->columnCount())
		return QVariant(_dataSet->column(column).columnType());

	return QVariant(-1);
}

bool DataSetPackage::columnHasFilter(int column) const
{
	if(_dataSet != nullptr && column >= 0 && size_t(column) < _dataSet->columnCount())
		return _dataSet->column(column).hasFilter();
	
	return false;
}


int DataSetPackage::columnsFilteredCount()
{
	if(_dataSet == nullptr) return 0;

	int colsFiltered = 0;

	for(auto & col : _dataSet->columns())
		if(col.hasFilter())
			colsFiltered++;

	return colsFiltered;
}

void DataSetPackage::resetAllFilters()
{
	for(auto & col : _dataSet->columns())
		col.resetFilter();

	emit allFiltersReset();
	emit columnsFilteredCountChanged();
	emit headerDataChanged(Qt::Horizontal, 0, columnCount());
}

bool DataSetPackage::setColumnType(int columnIndex, Column::ColumnType newColumnType)
{
	if (_dataSet == nullptr)
		return true;

	bool changed = _dataSet->column(columnIndex).changeColumnType(newColumnType);
	emit headerDataChanged(Qt::Horizontal, columnIndex, columnIndex);

	return changed;
}

void DataSetPackage::refreshColumn(Column * column)
{
	for(size_t col=0; col<_dataSet->columns().columnCount(); col++)
		if(&(_dataSet->columns()[col]) == column)
			emit dataChanged(index(0, col, parentModelForType(parIdxType::data)), index(rowCount()-1, col, parentModelForType(parIdxType::data)));
}

void DataSetPackage::columnWasOverwritten(std::string columnName, std::string possibleError)
{
	for(size_t col=0; col<_dataSet->columns().columnCount(); col++)
		if(_dataSet->columns()[col].name() == columnName)
			emit dataChanged(index(0, col, parentModelForType(parIdxType::data)), index(rowCount()-1, col, parentModelForType(parIdxType::data)));
}

int DataSetPackage::setColumnTypeFromQML(int columnIndex, int newColumnType)
{
	bool changed = setColumnType(columnIndex, Column::ColumnType(newColumnType));

	if (changed)
	{
		emit headerDataChanged(Qt::Orientation::Horizontal, columnIndex, columnIndex);
		emit columnDataTypeChanged(_dataSet->column(columnIndex).name());
	}

	return columnType(columnIndex);
}

void DataSetPackage::beginSynchingData()
{
	beginLoadingData();
	_dataSet->setSynchingData(true);
}

void DataSetPackage::endSynchingDataChangedColumns(std::vector<std::string>	&	changedColumns)
{
	 std::vector<std::string>				missingColumns;
	 std::map<std::string, std::string>		changeNameColumns;

	endSynchingData(changedColumns, missingColumns, changeNameColumns, false, false);
}

void DataSetPackage::endSynchingData(std::vector<std::string>			&	changedColumns,
									 std::vector<std::string>			&	missingColumns,
									 std::map<std::string, std::string>	&	changeNameColumns,
									 bool									rowCountChanged,
									 bool									hasNewColumns)
{
	_dataSet->setSynchingData(false);

	emit dataSynched(changedColumns, missingColumns, changeNameColumns, rowCountChanged, hasNewColumns);
	endLoadingData();
}


void DataSetPackage::beginLoadingData()
{
	_enginesLoadedAtBeginSync = !enginesInitializing();

	if(_enginesLoadedAtBeginSync)
		pauseEngines();

	beginResetModel();
}

void DataSetPackage::endLoadingData()
{
	endResetModel();

	if(_enginesLoadedAtBeginSync)
		resumeEngines();

	emit dataSetChanged();
}

void DataSetPackage::setDataSetSize(size_t columnCount, size_t rowCount)
{
	enlargeDataSetIfNecessary([&]()
	{
		_dataSet->setColumnCount(columnCount);

		if (rowCount > 0)
			_dataSet->setRowCount(rowCount);
	}, "setDataSetSize");
}

bool DataSetPackage::initColumnAsScale(size_t colNo, std::string newName, const std::vector<double> & values)
{
	bool out = false;

	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(colNo);
		column.setName(newName);
		out = column.setColumnAsScale(values);
	}, "initColumnAsScale");

	return out;
}

std::map<int, std::string> DataSetPackage::initColumnAsNominalText(size_t colNo, std::string newName, const std::vector<std::string> & values, const std::map<std::string, std::string> & labels)
{
	std::map<int, std::string> out;

	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(colNo);
		column.setName(newName);
		out = column.setColumnAsNominalText(values, labels);
	}, "initColumnAsNominalText");

	return out;
}

bool DataSetPackage::initColumnAsNominalOrOrdinal(size_t colNo, std::string newName, const std::vector<int> & values, const std::set<int> &uniqueValues, bool is_ordinal)
{
	bool out = false;

	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(colNo);
		column.setName(newName);
		out = column.setColumnAsNominalOrOrdinal(values, uniqueValues, is_ordinal);
	}, "initColumnAsNominalOrOrdinal");

	return out;
}

bool DataSetPackage::initColumnAsNominalOrOrdinal(size_t colNo, std::string newName, const std::vector<int> & values, const std::map<int, std::string> &uniqueValues, bool is_ordinal)
{
	bool out = false;

	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(colNo);
		column.setName(newName);
		out = column.setColumnAsNominalOrOrdinal(values, uniqueValues, is_ordinal);
	}, "initColumnAsNominalOrOrdinal");

	return out;
}

bool DataSetPackage::initColumnAsScale(QVariant colID, std::string newName, const std::vector<double> & values)
{
	if(colID.type() == QMetaType::Int || colID.type() == QMetaType::UInt)
	{
		int colNo = colID.type() == QMetaType::Int ? colID.toInt() : colID.toUInt();
		return initColumnAsScale(colNo, newName, values);
	}
	else
		return initColumnAsScale(colID.toString().toStdString(), newName, values);
}

std::map<int, std::string> DataSetPackage::initColumnAsNominalText(QVariant colID, std::string newName, const std::vector<std::string> & values, const std::map<std::string, std::string> & labels)
{
	if(colID.type() == QMetaType::Int || colID.type() == QMetaType::UInt)
	{
		int colNo = colID.type() == QMetaType::Int ? colID.toInt() : colID.toUInt();
		return initColumnAsNominalText(colNo, newName, values, labels);
	}
	else
		return initColumnAsNominalText(colID.toString().toStdString(), newName, values, labels);
}

bool DataSetPackage::initColumnAsNominalOrOrdinal(	QVariant colID, std::string newName, const std::vector<int> & values, const std::set<int> &uniqueValues, bool is_ordinal)
{
	if(colID.type() == QMetaType::Int || colID.type() == QMetaType::UInt)
	{
		int colNo = colID.type() == QMetaType::Int ? colID.toInt() : colID.toUInt();
		return initColumnAsNominalOrOrdinal(colNo, newName, values, uniqueValues, is_ordinal);
	}
	else
		return initColumnAsNominalOrOrdinal(colID.toString().toStdString(), newName, values, uniqueValues, is_ordinal);
}

bool DataSetPackage::initColumnAsNominalOrOrdinal(	QVariant colID, std::string newName, const std::vector<int> & values, const std::map<int, std::string> &uniqueValues, bool is_ordinal)
{
	if(colID.type() == QMetaType::Int || colID.type() == QMetaType::UInt)
	{
		int colNo = colID.type() == QMetaType::Int ? colID.toInt() : colID.toUInt();
		return initColumnAsNominalOrOrdinal(colNo, newName, values, uniqueValues, is_ordinal);
	}
	else
		return initColumnAsNominalOrOrdinal(colID.toString().toStdString(), newName, values, uniqueValues, is_ordinal);
}

void DataSetPackage::enlargeDataSetIfNecessary(std::function<void()> tryThis, const char * callerText)
{
	while(true)
	{
		try	{ tryThis(); return; }
		catch (boost::interprocess::bad_alloc &)
		{
			try							{ setDataSet(SharedMemory::enlargeDataSet(_dataSet)); }
			catch (std::exception &)	{ throw std::runtime_error("Out of memory: this data set is too large for your computer's available memory");	}
		}
		catch (std::exception & e)	{ Log::log() << "std::exception in enlargeDataSetIfNecessary for " << callerText << ": " << e.what() << std::endl;	}
		catch (...)					{ Log::log() << "something went wrong while enlargeDataSetIfNecessary for " << callerText << "..." << std::endl;	}

	}
}

std::vector<std::string> DataSetPackage::columnNames(bool includeComputed)
{
	std::vector<std::string> names;

	if(_dataSet)
		for(const Column & col : _dataSet->columns())
			if(includeComputed || !isColumnComputed(col.name()))
				names.push_back(col.name());

	return names;
}

bool DataSetPackage::isColumnDifferentFromStringValues(std::string columnName, std::vector<std::string> strVals)
{
	try
	{
		Column & col = _dataSet->column(columnName);
		return col.isColumnDifferentFromStringValues(strVals);
	}
	catch(columnNotFound & ) { }

	return true;
}

void DataSetPackage::renameColumn(std::string oldColumnName, std::string newColumnName)
{
	try
	{
		Column & col = _dataSet->column(oldColumnName);
		col.setName(newColumnName);
	}
	catch(...)
	{
		Log::log() << "Couldn't rename column from '" << oldColumnName << "' to '" << newColumnName << "'" << std::endl;
	}
}

void DataSetPackage::writeDataSetToOStream(std::ostream & out, bool includeComputed)
{
	std::vector<Column*> cols;

	int columnCount = _dataSet->columnCount();
	for (int i = 0; i < columnCount; i++)
	{
		Column &column		= _dataSet->column(i);
		std::string name	= column.name();

		if(!isColumnComputed(name) || includeComputed)
			cols.push_back(&column);
	}


	for (size_t i = 0; i < cols.size(); i++)
	{
		Column *column		= cols[i];
		std::string name	= column->name();

		if (stringUtils::escapeValue(name))	out << '"' << name << '"';
		else								out << name;

		if (i < cols.size()-1)	out << ",";
		else					out << "\n";

	}

	size_t rows = rowCount();

	for (size_t r = 0; r < rows; r++)
		for (size_t i = 0; i < cols.size(); i++)
		{
			Column *column = cols[i];

			std::string value = column->getOriginalValue(r);
			if (value != ".")
			{
				if (stringUtils::escapeValue(value))	out << '"' << value << '"';
				else									out << value;
			}

			if (i < cols.size()-1)		out << ",";
			else if (r != rows-1)		out << "\n";
		}
}

std::string DataSetPackage::getColumnTypeNameForJASPFile(Column::ColumnType columnType)
{
	switch(columnType)
	{
	case Column::ColumnTypeNominal:			return "Nominal";
	case Column::ColumnTypeNominalText:		return "NominalText";
	case Column::ColumnTypeOrdinal:			return "Ordinal";
	case Column::ColumnTypeScale:			return "Continuous";
	default:								return "Unknown";
	}
}

Column::ColumnType DataSetPackage::parseColumnTypeForJASPFile(std::string name)
{
	if (name == "Nominal")				return  Column::ColumnTypeNominal;
	else if (name == "NominalText")		return  Column::ColumnTypeNominalText;
	else if (name == "Ordinal")			return  Column::ColumnTypeOrdinal;
	else if (name == "Continuous")		return  Column::ColumnTypeScale;
	else								return  Column::ColumnTypeUnknown;
}

Json::Value DataSetPackage::columnToJsonForJASPFile(size_t columnIndex, Json::Value labelsData, size_t & dataSize)
{
	Column &column					= _dataSet->column(columnIndex);
	std::string name				= column.name();
	Json::Value columnMetaData		= Json::Value(Json::objectValue);
	columnMetaData["name"]			= Json::Value(name);
	columnMetaData["measureType"]	= Json::Value(getColumnTypeNameForJASPFile(column.columnType()));

	if (column.columnType()			!= Column::ColumnTypeScale)
	{
		columnMetaData["type"] = Json::Value("integer");
		dataSize += sizeof(int) * rowCount();
	}
	else
	{
		columnMetaData["type"] = Json::Value("number");
		dataSize += sizeof(double) * rowCount();
	}


	if (column.columnType() != Column::ColumnTypeScale)
	{
		Labels &labels = column.labels();
		if (labels.size() > 0)
		{
			Json::Value &columnLabelData	= labelsData[name];
			Json::Value &labelsMetaData		= columnLabelData["labels"];
			int labelIndex = 0;

			for (const Label &label : labels)
			{
				Json::Value keyValueFilterPair(Json::arrayValue);

				keyValueFilterPair.append(label.value());
				keyValueFilterPair.append(label.text());
				keyValueFilterPair.append(label.filterAllows());

				labelsMetaData.append(keyValueFilterPair);
				labelIndex += 1;
			}

			Json::Value &orgStringValuesMetaData	= columnLabelData["orgStringValues"];
			std::map<int, std::string> &orgLabels	= labels.getOrgStringValues();
			for (const std::pair<int, std::string> &pair : orgLabels)
			{
				Json::Value keyValuePair(Json::arrayValue);
				keyValuePair.append(pair.first);
				keyValuePair.append(pair.second);
				orgStringValuesMetaData.append(keyValuePair);
			}
		}
	}

	return columnMetaData;
}

void DataSetPackage::columnLabelsFromJsonForJASPFile(Json::Value xData, Json::Value columnDesc, size_t columnIndex, std::map<std::string, std::map<int, int> > & mapNominalTextValues)
{
	std::string name					= columnDesc["name"].asString();
	Json::Value &orgStringValuesDesc	= columnDesc["orgStringValues"];
	Json::Value &labelsDesc				= columnDesc["labels"];
	std::map<int, int>& mapValues		= mapNominalTextValues[name];	// This is needed for old JASP file where factor keys where not filled in the right way

	if (labelsDesc.isNull() &&  ! xData.isNull())
	{
		Json::Value &columnlabelData = xData[name];

		if (!columnlabelData.isNull())
		{
			labelsDesc			= columnlabelData["labels"];
			orgStringValuesDesc = columnlabelData["orgStringValues"];
		}
	}

	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(columnIndex);
		Column::ColumnType columnType = parseColumnTypeForJASPFile(columnDesc["measureType"].asString());

		column.setName(name);
		column.setColumnType(columnType);

		Labels &labels = column.labels();
		labels.clear();
		int index = 1;

		for (Json::Value keyValueFilterTrip : labelsDesc)
		{
			int zero		= 0; //MSVC complains on int(0) with: error C2668: 'Json::Value::get': ambiguous call to overloaded function
			int key			= keyValueFilterTrip.get(zero,		Json::nullValue).asInt();
			std::string val = keyValueFilterTrip.get(1,			Json::nullValue).asString();
			bool fil		= keyValueFilterTrip.get(2,			true).asBool();
			int labelValue	= key;

			if (columnType == Column::ColumnTypeNominalText)
			{
				labelValue		= index;
				mapValues[key]	= labelValue;
			}

			labels.add(labelValue, val, fil, columnType == Column::ColumnTypeNominalText);

			index++;
		}

		if (!orgStringValuesDesc.isNull())
		{
			for (Json::Value keyValuePair : orgStringValuesDesc)
			{
				int zero		= 0; //MSVC complains on int(0) with: error C2668: 'Json::Value::get': ambiguous call to overloaded function
				int key			= keyValuePair.get(zero,	Json::nullValue).asInt();
				std::string val = keyValuePair.get(1,		Json::nullValue).asString();
				if (mapValues.find(key) != mapValues.end())
					key = mapValues[key];
				else
					Log::log() << "Cannot find key " << key << std::flush;
				labels.setOrgStringValues(key, val);
			}
		}

	}, "columnLabelsFromJsonForJASPFile");

}

std::vector<int> DataSetPackage::getColumnDataInts(size_t columnIndex)
{
	if(_dataSet == nullptr) return {};

	Column & col = _dataSet->column(columnIndex);
	return std::vector<int>(col.AsInts.begin(), col.AsInts.end());
}

std::vector<double> DataSetPackage::getColumnDataDbls(size_t columnIndex)
{
	if(_dataSet == nullptr) return {};

	Column & col = _dataSet->column(columnIndex);
	return std::vector<double>(col.AsDoubles.begin(), col.AsDoubles.end());
}

void DataSetPackage::setColumnDataInts(size_t columnIndex, std::vector<int> ints)
{
	Column & col = _dataSet->column(columnIndex);
	Labels & lab = col.labels();

	for(size_t r = 0; r<ints.size(); r++)
	{
		int value = ints[r];

		//Maybe something went wrong somewhere and we do not have labels for all values...
		try
		{
			if (value != INT_MIN)
				lab.getLabelObjectFromKey(value);
		}
		catch (const labelNotFound &)
		{
			Log::log() << "Value '" << value << "' in column '" << col.name() << "' did not have a corresponding label, adding one now.\n";
			lab.add(value, std::to_string(value), true, col.columnType() == Column::ColumnTypeNominalText);
		}

		col.setValue(r, value);
	}
}


void DataSetPackage::setColumnDataDbls(size_t columnIndex, std::vector<double> dbls)
{
	Column & col = _dataSet->column(columnIndex);
	Labels & lab = col.labels();

	for(size_t r = 0; r<dbls.size(); r++)
		col.setValue(r, dbls[r]);
}

void DataSetPackage::emptyValuesChangedHandler()
{
	if (isLoaded())
	{
		beginSynchingData();
		std::vector<std::string> colChanged;

		enlargeDataSetIfNecessary([&](){ colChanged = _dataSet->resetEmptyValues(emptyValuesMap()); }, "emptyValuesChangedHandler");

		endSynchingDataChangedColumns(colChanged);
	}
}

bool DataSetPackage::setFilterData(std::string filter, std::vector<bool> filterResult)
{
	setDataFilter(filter);

	bool someFilterValueChanged = _dataSet ? _dataSet->setFilterVector(filterResult) : false;

	if(someFilterValueChanged) //We could also send exactly those cells that were changed if we were feeling particularly inclined to write the code...
	{
		emit dataChanged(index(0, 0, parentModelForType(parIdxType::filter)),	index(rowCount(), 0,				parentModelForType(parIdxType::filter)));
		emit dataChanged(index(0, 0, parentModelForType(parIdxType::data)),		index(rowCount(), columnCount(),	parentModelForType(parIdxType::data)));
	}

	return someFilterValueChanged;
}

Column::ColumnType DataSetPackage::columnType(std::string columnName)	const
{
	int colIndex = getColumnIndex(columnName);

	if(colIndex > -1)	return columnType(colIndex);
	else				return Column::ColumnTypeUnknown;
}

QStringList DataSetPackage::getColumnLabelsAsStringList(std::string columnName)	const
{
	int colIndex = getColumnIndex(columnName);

	if(colIndex > -1)	return getColumnLabelsAsStringList(colIndex);
	else				return QStringList();;
}

QStringList DataSetPackage::getColumnLabelsAsStringList(size_t columnIndex)	const
{
	QStringList list;
	if(columnIndex < 0 || columnIndex >= columnCount()) return list;

	for(const Label & label : _dataSet->column(columnIndex).labels())
		list.append(tq(label.text()));

	return list;
}
