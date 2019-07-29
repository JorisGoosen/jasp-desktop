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

#define ENUM_DECLARATION_CPP
#include "datasetpackage.h"
#include "utilities/qutils.h"

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
	//emit dataSetChanged(_dataSet);
}

parIdxType DataSetPackage::parentModelIndexIs(const QModelIndex &index) const
{
	if(index.isValid())
		switch(index.row())
		{
		case 0:	return parIdxType::data;
		case 1: return parIdxType::label; //col is actually columnIndex of label
		case 2: return parIdxType::filter;
		}
	return parIdxType::root;
}

QModelIndex DataSetPackage::parentModelForType(parIdxType type, int column) const
{
	switch(type)
	{
	case parIdxType::data:		return index(0, column, QModelIndex());
	case parIdxType::label:		return index(1, column, QModelIndex());
	case parIdxType::filter:	return index(2, column, QModelIndex());
	}

	return QModelIndex(); // parIdxType::root
}

bool				hasChildren(const QModelIndex &index)												const	override;

int DataSetPackage::rowCount(const QModelIndex & parent) const
{
	switch(parentModelIndexIs(parent))
	{
	case parIdxType::root:		return 3;
	case parIdxType::label:		return _dataSet == nullptr ? 0 : _dataSet->columnCount() > parent.column() ? _dataSet->columns()[parent.column()].labels().size() : 0;
	case parIdxType::filter:
	case parIdxType::data:		return _dataSet == nullptr ? 0 : _dataSet->rowCount();
	}

	return 0; // <- because gcc is stupid
}

int DataSetPackage::columnCount(const QModelIndex &parent) const
{
	switch(parentModelIndexIs(parent))
	{
	case parIdxType::root:
	case parIdxType::filter:	return 1;
	case parIdxType::label:
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
	switch(parentModelIndexIs(index.parent()))
	{
	case parIdxType::root: should do something about root I guess?
	case parIdxType::filter:
		if(_dataSet == nullptr || index.column() < 0 || index.column() >= _dataSet->filterVector().size())
			return true;
		return _dataSet->filterVector()[index.column()];

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
					right	= iAmActive && index.column() == columnCount() - 1; //always draw left line and right line only if last col

			return			(left ?		1 : 0) +
							(right ?	2 : 0) +
							(up ?		4 : 0) +
							(down ?		8 : 0);
		}
		}

	case parIdxType::label:
		if(_dataSet == nullptr || index.column() >= _dataSet->columnCount() || index.row() >= rowCount(parentModelForType(parIdxType::label, index.column())))
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
		QString dummyText = headerData(section, orientation, Qt::DisplayRole).toString() + "XXXXX" + (isComputedColumn(section) ? "XXXXX" : ""); //Bit of padding for filtersymbol and columnIcon
		int colWidth = _dataSet->getMaximumColumnWidthInCharacters(section);

		while(colWidth > dummyText.length())
			dummyText += "X";

		return dummyText;
	}
	case Qt::DisplayRole:									return orientation == Qt::Horizontal ? tq(_dataSet->column(section).name()) : QVariant(section + 1);
	case Qt::TextAlignmentRole:								return QVariant(Qt::AlignCenter);
	case int(specialRoles::columnIsComputed):				return isComputedColumn(section);
	case int(specialRoles::computedColumnIsInvalidated):	return isComputedColumnInvalided(section);
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
	dataSet()->setSynchingData(true);
}

void DataSetPackage::endSynchingData(std::vector<std::string>			&	changedColumns,
									 std::vector<std::string>			&	missingColumns,
									 std::map<std::string, std::string>	&	changeNameColumns,
									 bool									rowCountChanged,
									 bool									hasNewColumns)
{
	dataSet()->setSynchingData(false);

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
