#include "timers.h"
#include "columnq.h"
#include "filterq.h"
#include "datasetq.h"
#include "variableinfo.h"
#include "columnencoder.h"
#include "utilities/qutils.h"
#include "modules/ribbonmodel.h"
#include "../datasetpackageenums.h"

DataSetQ::DataSetQ(int index) 
	: DataSet(index), QAbstractTableModel(nullptr)
{
	setModifiedCallback		( [this](){	emit somethingModified();		});	
	_descriptionChanged		= [this](){	emit descriptionChanged();		};
	_dataFilePathChanged	= [this](){	emit dataFileChanged();			};
	_databaseJsonChanged	= [this](){	emit databaseJsonChanged();		};
	_dataSynchChanged		= [this](){	emit dataFileSynchChanged();	};
	_dataTimestampChanged	= [this](){	emit dataTimestampChanged();	};

	connect(this, &DataSetQ::datasetChanged,	this, &DataSetQ::handleDataSetChanged);
}

DataSetQ::~DataSetQ()
{
	
}

QHash<int, QByteArray> DataSetQ::roleNames() const
{
	static bool						set = false;
	static QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();

	if(!set)
	{
		for(const auto & enumString : dataPkgRolesToStringMap())
			roles[int(enumString.first)] = QString::fromStdString(enumString.second).toUtf8();

		set = true;
	}

	return roles;
}

int DataSetQ::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : DataSet::rowCount();
}

int DataSetQ::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : DataSet::columnCount();
}

QVariant DataSetQ::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
		return QVariant();
	
	
	if(index.row() >= rowCount() || index.column() >= columnCount())
		return QVariant(); // if there is no data then it doesn't matter what role we play
	
	JASPTIMER_SCOPE(DataSetQ::data);
	
	Column * column = columns()[index.column()];

	switch(role)
	{
	case Qt::DisplayRole:									return tq(column->getDisplay(index.row()));
	case int(dataPkgRoles::label):							return tq(column->getLabel(index.row(), false, true));
	case int(dataPkgRoles::value):							return tq(column->getValue(index.row()));
	case int(dataPkgRoles::name):							return tq(column->name());
	case int(dataPkgRoles::title):							return tq(column->title());
	case int(dataPkgRoles::filter):							return getRowFilter(index.row());
	case int(dataPkgRoles::columnType):						return int(column->type());
	case int(dataPkgRoles::description):					return tq(column->description());
	case int(dataPkgRoles::inEasyFilter):					return getColumnInDragNDropShownFilter(column);
	case int(dataPkgRoles::shadowDisplay):					return tq(column->getShadow(index.row()));
	case int(dataPkgRoles::valuesDblList):					return getColumnValuesAsDoubleList(getColumnIndex(column->name()));
	case int(dataPkgRoles::nonFilteredNumericValuesCount):	return column->nonFilteredNumericsCount();
	case int(dataPkgRoles::nonFilteredLevels):				return tq(column->nonFilteredLevels());
	case int(dataPkgRoles::computedColumnType):				return int(column->codeType());
	case int(dataPkgRoles::columnPkgIndex):					return index.column();
	case int(dataPkgRoles::lines):
	{
		bool	iAmActive		= getRowFilter(index.row()),
				belowMeIsActive = index.row() < column->rowCount() - 1	&& data(this->index(index.row() + 1, index.column()), int(dataPkgRoles::filter)).toBool();

		return getDataSetViewLines(
			iAmActive,
			iAmActive,
			iAmActive && !belowMeIsActive,
			iAmActive && index.column() == columnCount(index.parent()) - 1 //always draw left line and right line only if last col
		);
	}
	}
	
	return QVariant();
}

bool DataSetQ::setData(const QModelIndex &index, const QVariant &value, int role)
{
	JASPTIMER_SCOPE(DataSetQ::setData);
		
	if(!index.isValid() || index.column() < 0 || index.column() >= columnCount()) 
		return false;

	ColumnQ	* column	= static_cast<ColumnQ*>(columns()[index.column()]);
	
	if(role == Qt::DisplayRole || role == Qt::EditRole || role == int(dataPkgRoles::value) || role == int(dataPkgRoles::valueLabelPair) || role == int(dataPkgRoles::valuesStrList))
	{				
		bool				isPair	= role == int(dataPkgRoles::valueLabelPair),
							isVals	= role == int(dataPkgRoles::valuesStrList);
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
			JASPTIMER_SCOPE(ColumnQ::setData reset model);

			emit manualEditMade();
			
			column->labelsRemoveOrphans();
			column->labelsTempReset();
			column->labelsHandleAutoSort();

			refresh();
			emit handleColumnChanged(column);
			emit handleLabelsReordered(column);
			
			//Probably the labelfilter thing and the constructor thing should 
			if(column->hasLabelFilter())
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
		case int(dataPkgRoles::description):
			column->setDescription(value.toString().toStdString());
			aChange = true;
			break;

		case int(dataPkgRoles::title):
			column->setTitle(value.toString().toStdString());
			aChange = true;
			break;

		case int(dataPkgRoles::columnType):
			if(value.toInt() >= int(columnType::unknown) && value.toInt() <= int(columnType::scale))
			{
				columnType converted = static_cast<columnType>(value.toInt());
				if(converted != column->type())
				{
					if(column->changeType(converted) == columnTypeChangeResult::generatedFromAnalysis)
						emit showWarning(tr("Changing column type failed"), tr("The column '%1' is generated by an analysis and its type is fixed.").arg(tq(column->name())));
					else
					{
						aChange = true;
						emit handleColumnTypeChanged(column);
					}
				}
			}
			break;
		}

		if(aChange)
		{
			refresh();
			emit manualEditMade();
		}
		return true;
	}	
}

QVariant DataSetQ::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (section < 0 || section >= (orientation == Qt::Horizontal ? columnCount() : rowCount()))
			return QVariant();
		
	JASPTIMER_SCOPE(DataSetQ::headerData);
	
	if(orientation == Qt::Vertical)
		switch(role)
		{
		default:
			return QVariant();

		case int(dataPkgRoles::maxRowHeaderString):
			return QString::number(rowCount()) + "XXX";

		case Qt::DisplayRole:
			return QVariant(section + 1);
		}
	else
	{
		Column * col = columns()[section];
				
		switch(role)
		{
		case int(dataPkgRoles::maxColString):
		{
			//calculate some kind of maximum string to give views an expectation of the width needed for a column
			bool		hasFilter	= col && (col->hasLabelFilter() || getColumnInDragNDropShownFilter(col));
			QString		dummyText	= headerData(section, orientation, int(dataPkgRoles::maxColumnHeaderString)).toString() + (col->isComputed() ? "XXX" : "") + (hasFilter ? "XXX" : ""); //Bit of padding for hamburger, filtersymbol and columnIcon
			qsizetype	colWidth	= getMaximumColumnWidthInCharacters(section);

			while(colWidth > dummyText.length())
				dummyText += "X";

			return dummyText;
		}
		case int(dataPkgRoles::maxColumnHeaderString):			return headerData(section, orientation, Qt::DisplayRole).toString() + "XXX";
		case int(dataPkgRoles::maxRowHeaderString):				return QString::number(rowCount())		+ "XXX";
		case Qt::TextAlignmentRole:								return QVariant(Qt::AlignCenter);
		case int(dataPkgRoles::filter):							return		!col ? false							: col->hasLabelFilter() || getColumnInDragNDropShownFilter(col);
		case Qt::DisplayRole:									return tq(	!col ? "?"								: col->name());
		
		case int(dataPkgRoles::labelsHasFilter):				return		!col ? false							: col->hasLabelFilter();
		case int(dataPkgRoles::columnIsComputed):				return		!col ? false							: col->isComputed() && col->codeType() != computedColumnType::analysisNotComputed;
		case int(dataPkgRoles::computedColumnError):			return tq(	!col ? "?"								: col->error());
		case int(dataPkgRoles::computedColumnIsInvalidated):	return		!col ? false							: col->invalidated();
		case int(dataPkgRoles::columnType):						return int(	!col ? columnType::unknown				: col->type());
		case int(dataPkgRoles::computedColumnType):				return int(	!col ? computedColumnType::notComputed	: col->codeType());
		case int(dataPkgRoles::description):					return tq(	!col ? "?"								: col->description());
		case int(dataPkgRoles::title):							return tq(	!col ? "?"								: col->title());
		case int(dataPkgRoles::previewScale):
		case int(dataPkgRoles::previewOrdinal):					
		case int(dataPkgRoles::previewNominal):					
		{
			columnType colTypeWanted = 
					role == int(dataPkgRoles::previewNominal) 
					? columnType::nominal 
					: role == int(dataPkgRoles::previewOrdinal)
					? columnType::ordinal
					: columnType::scale;
			
			stringvec preview = !col ? stringvec() : col->previewTransform(colTypeWanted);
			
			if(preview.size() != 4)
				return QVariant();
			
			QString	levelsTotal		= tq(preview[0]),
					levelsNums		= tq(preview[1]),
					vals			= tq(preview[2]),
					empties			= tq(preview[3]);
			
			if(colTypeWanted == columnType::scale)
				return	tr("There are %1 total levels, of which %2 have a numeric value.\nAs a '%3' it looks like: %4\n%5")
						.arg(levelsTotal)
						.arg(levelsNums)
						.arg(VariableInfo::getTypeFriendly(colTypeWanted))
						.arg(vals)
						.arg(
							empties == "" 
							? "" 
							: tr("Implicit missing values: %1").arg(empties)
						);
			else
				return tr("There are %1 total levels.\nAs a '%2' it looks like: %3")
					.arg(levelsTotal)
					.arg(VariableInfo::getTypeFriendly(colTypeWanted))
					.arg(vals);
		}
		}
	}
	
	return QVariant();
}

Qt::ItemFlags DataSetQ::flags(const QModelIndex &index) const
{
	bool	isEditable	= dataMode() && index.column() >= 0 && index.column() < columnCount() && columns()[index.column()]->isComputed();

	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | (isEditable ? Qt::ItemIsEditable : Qt::NoItemFlags);
}

bool DataSetQ::insertRows(int row, int count, const QModelIndex &aparent)
{
	if(row > rowCount())
			row = rowCount();
	
	emit manualEditMade();
	
	beginInsertRows(QModelIndex(), row, row + count - 1);

	stringvec changed;

	beginBatchedToDB();
	for(int c=0; c<columnCount(); c++)
	{
		changed.push_back(column(c)->name());

		for(int r=row; r<row+count; r++)
			column(c)->rowInsertEmptyVal(r);
	}

	setRowCount(rowCount() + count);
	incRevision();
	endBatchedToDB();
	
	endInsertRows();

	strstrmap		changeNameColumns;
	stringvec		missingColumns;

	emit datasetChanged(tq(changed), tq(missingColumns), tq(changeNameColumns), true, false);

	return true;
}

bool DataSetQ::removeRows(int row, int count, const QModelIndex &aparent)
{
	if(row == -1)
		return false;
	
	emit manualEditMade();
	
	beginRemoveRows(QModelIndex(), row, row + count - 1);

	stringvec changed;

	beginBatchedToDB();
	
	for(Column * column : columns())
	{
		changed.push_back(column->name());
		
		for(int r=row+count; r>row; r--)
			column->rowDelete(r-1);
	}

	setRowCount(rowCount() - count);
	incRevision();
	endBatchedToDB();

	strstrmap		changeNameColumns;
	stringvec		missingColumns;

	endRemoveRows();

	emit datasetChanged(tq(changed), tq(missingColumns), tq(changeNameColumns), true, false);

	return true;
}

bool DataSetQ::isColumnNameFree(const std::string & name) const
{
	return getColumnIndex(name) == -1;	
}

std::string DataSetQ::freeNewColumnName(size_t startAfterThis) const
{
	const QString nameBase = tr("Column %1");

	while(true)
	{
		const std::string & newColName = fq(nameBase.arg(++startAfterThis));
		if(isColumnNameFree(newColName))
			return newColName;
	}
}

bool DataSetQ::insertColumns(int column, int count, const QModelIndex &aparent)
{
	if(column > columnCount())
			column = columnCount(); //the column will be created if necessary but only if it is in a logical place. So the end of the vector
	
	emit manualEditMade();
	
	beginInsertColumns(QModelIndex(), column, column + count - 1);
	
	stringvec changed;

	for(int c = column; c<column+count; c++)
	{
		const std::string & name = freeNewColumnName(c);
		
		DataSet::insertColumn(c);
		DataSet::column(c)->setName(name);
		DataSet::column(c)->setDefaultValues(columnType::scale);

		changed.push_back(name);
	}

	endInsertColumns();

	strstrmap		changeNameColumns;
	stringvec		missingColumns;

	emit datasetChanged(tq(changed), tq(missingColumns), tq(changeNameColumns), true, false);

	ColumnEncoder::setCurrentColumnNames(getColumnNames());

	return true;
}

bool DataSetQ::removeColumns(int column, int count, const QModelIndex &aparent)
{
	if(column == -1)
		return false;

	emit manualEditMade();
	
	beginRemoveColumns(QModelIndex(), column, column + count - 1);

	stringvec	changed;
	strstrmap	changeNameColumns;
	stringvec	missingColumns;

	for(int c = column + count; c>column; c--)
	{
		missingColumns.push_back(columns()[c - 1]->name());
		DataSet::removeColumn(c - 1);
	}

	endRemoveColumns();

	emit datasetChanged(tq(changed), tq(missingColumns), tq(changeNameColumns), false, true);

	ColumnEncoder::setCurrentColumnNames(getColumnNames());

	return true;
}

bool DataSetQ::dataMode()
{
	return RibbonModel::singleton()->dataMode();
}

ColumnsQ & DataSetQ::columnsQ() const
{
	//Not sure if this is a performance problem, but saves me casting a lot in other places so lets see
	JASPTIMER_SCOPE(DataSetQ::columnsQ);

	static ColumnsQ howBadWouldThisBe;

	howBadWouldThisBe.clear();
	howBadWouldThisBe.reserve(columns().size());
	std::transform(columns().begin(), columns().end(), howBadWouldThisBe.begin(), [](Column * c){ return static_cast<ColumnQ*>(c); });

	return howBadWouldThisBe;
}

void DataSetQ::handleColumnChanged(ColumnQ *column)
{
	emit datasetChanged(tq(stringvec({column->name()})), {}, {}, false, false);
}

void DataSetQ::handleLabelsReordered(ColumnQ *column)
{
	emit labelsReordered(tq(column->name()));
}

void DataSetQ::handleColumnTypeChanged(ColumnQ *column)
{
	emit columnTypeChanged(tq(column->name()));
}

void DataSetQ::handleDataSetChanged(QStringList				changedColumns,
									QStringList				missingColumns,
									QMap<QString, QString>	changeNameColumns,
									bool					rowCountChanged,
									bool					hasNewColumns)
{
	if(rowCountChanged)
		DataSetQ::rowCountChanged();
}

bool DataSetQ::getRowFilter(int row) const
{
	return bool(shownFilter()->filtered().at(row));
}

QVariant DataSetQ::getDataSetViewLines(bool up, bool left, bool down, bool right) const
{
	return			(left ?		1 : 0) +
					(right ?	2 : 0) +
					(up ?		4 : 0) +
			(down ?		8 : 0);
}

bool DataSetQ::getColumnInDragNDropShownFilter(int columnIndex) const
{
	if(columnIndex < 0 || columnIndex >= columnCount()) 
		return false;
	
	return getColumnInDragNDropShownFilter(columns()[columnIndex]);
}

bool DataSetQ::getColumnInDragNDropShownFilter(Column * column) const
{
	return shownFilter()->columnsUsedInConstructor().count(column->name());
}

Column * DataSetQ::connectNewColumn(ColumnQ * newColumn)
{
	connect(newColumn, &ColumnQ::manualEditMade,		this, &DataSetQ::manualEditMade	);
	connect(newColumn, &ColumnQ::dataSetShouldRefresh,	this, &DataSetQ::refresh		);
	
	return newColumn;
}

Filter *DataSetQ::connectNewFilter(FilterQ *newFilter)
{
	connect(newFilter, &FilterQ::dataSetShouldRefresh, this, &DataSetQ::refresh);

	return newFilter;
}

Column * DataSetQ::_createColumn(int id)
{
	return connectNewColumn(new ColumnQ(this, id));
}

Filter *DataSetQ::_createFilter()
{
	return connectNewFilter(new FilterQ(this));
}

Filter *DataSetQ::_createFilter(const std::string &name, bool createIfMissing)
{
	return connectNewFilter(new FilterQ(this, name, createIfMissing));
}

QString DataSetQ::descriptionQ() const
{
	return tq(description());
}

void DataSetQ::setDescriptionQ(const QString & newDescription)
{
	setDescription(fq(newDescription));
}

QString DataSetQ::dataFileQ() const
{
	return tq(dataFilePath());
}

void DataSetQ::setDataFileQ(const QString &newDataFile)
{
	setDataFile(fq(newDataFile));
}

QString DataSetQ::databaseJsonQ() const
{
	return tq(databaseJson());
}

void DataSetQ::setDatabaseJsonQ(const QString &newDatabaseJson)
{
	setDatabaseJson(fq(newDatabaseJson));
}

bool DataSetQ::dataFileSynchQ() const
{
	return dataFileSynch();
}

void DataSetQ::setDataFileSynchQ(bool newDataFileSynch)
{
	setDataFileSynch(newDataFileSynch);
}


bool DataSetQ::dataFileCanHaveLabels() const
{
	return !tq(dataFilePath()).endsWith(".csv");
}

void DataSetQ::resetAllFilters()
{
	for(Column * col : columns())
		col->resetFilter();

	resetFilterCounters();

	emit allFiltersReset();
	emit columnsFilteredCountChanged();
	//this is only used in conjunction with a reset so dont do: emit headerDataChanged(Qt::Horizontal, 0, columnCount());
}

void DataSetQ::resetFilterAllows(size_t columnIndex)
{
	column(columnIndex)->resetFilter();
	resetFilterCounters();

	emit labelFilterChanged();

	QModelIndex parentModel = indexForSubNode(_dataSet->dataNode());
	emit dataChanged(DataSetPackage::index(0, columnIndex,	parentModel),	DataSetPackage::index(rowCount() - 1, columnIndex, parentModel), {int(dataPkgRoles::filter)} );

	parentModel = indexForSubNode(_dataSet->column(columnIndex));
	emit dataChanged(DataSetPackage::index(0, 0,	parentModel),			DataSetPackage::index(rowCount(parentModel) - 1, columnCount(parentModel) - 1, parentModel), {int(dataPkgRoles::filter)} );


	emit filteredOutChanged(columnIndex);
}


int DataSetQ::columnsFilteredCount() const
{
	int colsFiltered = 0;

	for(Column * col : columns())
		if(col->hasLabelFilter())
			colsFiltered++;

	return colsFiltered;
}

void DataSetQ::resetFilterCounters()
{
	for(Column * col : columns())
		col->nonFilteredCountersReset();
}


bool DataSetQ::setColumnTypes(intset columnIndexes, columnType newColumnType)
{
	bool somethingChanged = false;

	for(int columnIndex : columnIndexes)
	{
		Column *col = column(columnIndex);

		if (col->type() == newColumnType)
			continue;


		//the only possible "fail" is when an analysis made the column and thus decides the type
		//the user might bet
		if(col->changeType(newColumnType) == columnTypeChangeResult::generatedFromAnalysis)
		{
			emit showWarning(tr("Changing column type failed"), tr("The column '%1' is generated by an analysis and its type is fixed.").arg(tq(col->name())));
			continue;
		}

		emit columnTypeChanged(tq(column(columnIndex)->name()));
		somethingChanged = true;
	}

	if(somethingChanged)
		refreshWithDelay();

	return somethingChanged;
}

void DataSetQ::resetVariableTypes()
{
	for (Column * col : _dataSet->columns())
	{
		columnType guessedType = col->resetValues(PreferencesModel::prefs()->thresholdScale());

		if(guessedType != col->type() && col->changeType(guessedType) == columnTypeChangeResult::changed)
		{
			emit columnDataTypeChanged(tq(col->name()));
			refreshWithDelay();
		}
	}
}
