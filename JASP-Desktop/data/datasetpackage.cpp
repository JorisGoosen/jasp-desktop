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

#define ENUM_DECLARATION_CPP
#include "datasetpackage.h"

parIdxType DataSetPackage::_nodeCategory[] = { parIdxType::root, parIdxType::data, parIdxType::label, parIdxType::filter, parIdxType::leaf };	//Should be same order as defined

DataSetPackage::DataSetPackage(QObject * parent) : QAbstractItemModel(parent)
{
	reset();
	informComputedColumnsOfPackage();
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
	setDataSet(SharedMemory::createDataSet());
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

	if(parentType == parIdxType::leaf)
		Log::log() << "DataSetPackage just had an index that had parIdxType::leaf as parent... That should NOT happen!" << std::endl;

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
	case parIdxType::leaf:		return 1; //Shouldnt really be called anyway
	case parIdxType::label:		return _dataSet == nullptr ? 0 : _dataSet->columnCount() > parent.column() ? _dataSet->columns()[parent.column()].labels().size() : 0;
	case parIdxType::filter:
	case parIdxType::root:		//return int(parIdxType::leaf); Its more logical to get the actual datasize
	case parIdxType::data:		return _dataSet == nullptr ? 0 : _dataSet->rowCount();
	}

	return 0; // <- because gcc is stupid
}

int DataSetPackage::columnCount(const QModelIndex &parent) const
{
	switch(parentIndexTypeIs(parent))
	{
	case parIdxType::leaf:
	case parIdxType::filter:	return 1;
	case parIdxType::label:
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
					belowMeIsActive = index.row() < rowCount() - 1	&& data(this->index(index.row() + 1, index.column()), int(specialRoles::filter)).toBool();


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
		if(_dataSet == nullptr || index.column() >= columnCount(index.parent()) || index.row() >= rowCount(index.parent()))
			return QVariant(); // if there is no data then it doesn't matter what role we play

		switch(role)
		{
		case int(specialRoles::value):				return tq(_dataSet->column(index.column()).labels().getValueFromRow(index.row()));
		case Qt::DisplayRole:
		default:									return tq(_dataSet->column(index.column()).labels().getLabelFromRow(index.row()));
		}
	}

	return QVariant(); // <- because gcc is stupid
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
		int colWidth = _dataSet->getMaximumColumnWidthInCharacters(section);

		while(colWidth > dummyText.length())
			dummyText += "X";

		return dummyText;
	}
	case Qt::DisplayRole:									return orientation == Qt::Horizontal ? tq(_dataSet->column(section).name()) : QVariant(section + 1);
	case Qt::TextAlignmentRole:								return QVariant(Qt::AlignCenter);
	case int(specialRoles::columnIsComputed):				return isColumnComputed(section);
	case int(specialRoles::computedColumnIsInvalidated):	return isColumnInvalidated(section);
	case int(specialRoles::columnIsFiltered):				return columnHasFilter(section) || columnUsedInEasyFilter(section);
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
		roles[int(specialRoles::columnIsFiltered)]				= QString("columnIsFiltered").toUtf8();
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
	{
		QString value = tq(_dataSet->column(column).name());
		return QVariant(value);
	}
	else
		return QVariant();
}

QVariant DataSetPackage::columnIcon(int column) const
{
	if(_dataSet != nullptr && column >= 0 && size_t(column) < _dataSet->columnCount())
	{
		Column &columnref = _dataSet->column(column);
		return QVariant(columnref.columnType());
	}
	else
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

Column::ColumnType DataSetPackage::getColumnType(int columnIndex)
{
	return _dataSet->column(columnIndex).columnType();
}

void DataSetPackage::refreshColumn(Column * column)
{
	for(size_t col=0; col<_dataSet->columns().columnCount(); col++)
		if(&(_dataSet->columns()[col]) == column)
			emit dataChanged(index(0, col), index(rowCount()-1, col));
}

void DataSetPackage::columnWasOverwritten(std::string columnName, std::string possibleError)
{
	for(size_t col=0; col<_dataSet->columns().columnCount(); col++)
		if(_dataSet->columns()[col].name() == columnName)
			emit dataChanged(index(0, col), index(rowCount()-1, col));
}

int DataSetPackage::setColumnTypeFromQML(int columnIndex, int newColumnType)
{
	bool changed = setColumnType(columnIndex, (Column::ColumnType)newColumnType);

	if (changed)
	{
		emit headerDataChanged(Qt::Orientation::Horizontal, columnIndex, columnIndex);
		emit columnDataTypeChanged(_dataSet->column(columnIndex).name());
	}

	return getColumnType(columnIndex);
}

void DataSetPackage::beginSynchingData()
{
	beginLoadingData();
	_dataSet->setSynchingData(true);
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

	emit dataSetChanged(_dataSet);
}

void DataSetPackage::setDataSetSize(int columnCount, int rowCount)
{
	enlargeDataSetIfNecessary([&]()
	{
		_dataSet->setColumnCount(columnCount);

		if (rowCount > 0)
			_dataSet->setRowCount(rowCount);
	});
}

bool DataSetPackage::initColumnAsScale(int colNo, std::string newName, const std::vector<double> & values)
{
	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(colNo);
		column.setName(newName);
		column.setColumnAsScale(values);
	});

}

std::map<int, std::string> DataSetPackage::initColumnAsNominalText(int colNo, std::string newName, const std::vector<std::string> & values)
{
	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(colNo);
		column.setName(newName);
		column.setColumnAsNominalText(values);
	});
}

bool DataSetPackage::initColumnAsNominalOrOrdinal(int colNo, std::string newName, const std::vector<int> & values, const std::set<int> &uniqueValues, bool is_ordinal)
{
	enlargeDataSetIfNecessary([&]()
	{
		Column &column = _dataSet->column(colNo);
		column.setName(newName);
		column.setColumnAsNominalOrOrdinal(values, uniqueValues, is_ordinal);
	});
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

std::map<int, std::string> DataSetPackage::initColumnAsNominalText(		QVariant colID,			std::string newName, const std::vector<std::string>	& values)
{
	if(colID.type() == QMetaType::Int || colID.type() == QMetaType::UInt)
	{
		int colNo = colID.type() == QMetaType::Int ? colID.toInt() : colID.toUInt();
		return initColumnAsNominalText(colNo, newName, values);
	}
	else
		return initColumnAsNominalText(colID.toString().toStdString(), newName, values);
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


void DataSetPackage::enlargeDataSetIfNecessary(std::function<void()> tryThis)
{
	while(true)
	{
		try	{ tryThis(); return; }
		catch (boost::interprocess::bad_alloc &)
		{
			try							{ setDataSet(SharedMemory::enlargeDataSet(_dataSet)); }
			catch (std::exception &)	{ throw std::runtime_error("Out of memory: this data set is too large for your computer's available memory");	}
		}
		catch (std::exception & e)	{ Log::log() << "std::exception in enlargeDataSetIfNecessary: " << e.what() << std::endl;	}
		catch (...)					{ Log::log() << "something went wrong while enlargeDataSetIfNecessary " << std::endl;		}

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

void DataSetPackage::writeDataSetToOStream(std::ostream out, bool includeComputed)
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

	size_t rowCount = rowCount();

	for (size_t r = 0; r < rowCount; r++)
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
			else if (r != rowCount-1)	out << "\n";
		}
}
