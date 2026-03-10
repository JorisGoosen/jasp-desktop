#include "expanddataproxymodel.h"
#include "datasettablemodel.h"
#include "dataenums.h"
#include "qutils.h"

ExpandDataProxyModel::ExpandDataProxyModel(QObject *parent)
	: QIdentityProxyModel{parent}
{
	connect(undoStack(), &QUndoStack::indexChanged, this, &ExpandDataProxyModel::undoChanged) ;
}

int ExpandDataProxyModel::rowCount(const QModelIndex &) const
{
	if (!dataSetSourceModel())
		return 0;
	return dataSetSourceModel()->rowCount() + (_expandDataSet ? EXTRA_ROWS : 0);
}

int ExpandDataProxyModel::columnCount(const QModelIndex &) const
{
	if (!dataSetSourceModel())
		return 0;
	return dataSetSourceModel()->columnCount() + (_expandDataSet ? EXTRA_COLS : 0);
}

QVariant ExpandDataProxyModel::data(const QModelIndex &indexP, int role) const
{
	if (!dataSetSourceModel() || role == -1) // Role not defined
		return QVariant();

	int row		= indexP.row(),
		column	= indexP.column();

	if (column < dataSetSourceModel()->columnCount() && row < dataSetSourceModel()->rowCount())
		return dataSetSourceModel()->data(dataSetSourceModel()->index(row, column), role);

	switch(role)
	{
	case int(dataPkgRoles::selected):				return false;
	case int(dataPkgRoles::lines):
	{
		DataSetTableModel * dataSetTable = dynamic_cast<DataSetTableModel *>(dataSetSourceModel());

		if (row < dataSetSourceModel()->rowCount() && dataSetTable && dataSetTable->showInactive() && !data(index(row,0), int(dataPkgRoles::filter)).toBool())
			return DataSet::getDataSetViewLines(false, false, false, false);
		return DataSet::getDataSetViewLines(column>0, row>0, true, true);
	}
	case int(dataPkgRoles::value):					return "";
	case int(dataPkgRoles::columnType):				return int(columnType::scale);
	default:										return QVariant();
	}

	return QVariant(); //gcc might complain some more I guess?
}

QVariant ExpandDataProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (!dataSetSourceModel() || role == -1) // Role not defined
		return QVariant();

	if (orientation == Qt::Orientation::Horizontal)
	{
		if (section < dataSetSourceModel()->columnCount())
			return dataSetSourceModel()->headerData(section, orientation, role);
		else
			switch(role)
			{
			case int(dataPkgRoles::columnIsComputed):				return false;
			case int(dataPkgRoles::computedColumnIsInvalidated):	return false;
			case int(dataPkgRoles::filter):							return false;
			case int(dataPkgRoles::computedColumnError):			return "";
			case int(dataPkgRoles::columnType):						return int(columnType::unknown);
			case int(dataPkgRoles::maxColString):					return "XXXXXXXXXXX";
			default:												return "";
			}
	}
	else if (orientation == Qt::Orientation::Vertical)
	{
		if (section < dataSetSourceModel()->rowCount())
			return dataSetSourceModel()->headerData(section, orientation, role);
		else if (section == 0 && role == int(dataPkgRoles::maxRowHeaderString))
			return "XXXX";
		else
			return  section + 1;
	}

	return QVariant();
}

Qt::ItemFlags ExpandDataProxyModel::flags(const QModelIndex &index) const
{
	if (!dataSetSourceModel())
		return Qt::NoItemFlags;

	if (index.column() < dataSetSourceModel()->columnCount() && index.row() < dataSetSourceModel()->rowCount())
		return dataSetSourceModel()->flags(dataSetSourceModel()->index(index.row(), index.column()));

	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QModelIndex ExpandDataProxyModel::index(int row, int column, const QModelIndex &) const
{
	if (!dataSetSourceModel())
		return QModelIndex();

	return createIndex(row, column);
}

QModelIndex ExpandDataProxyModel::parent(const QModelIndex &index) const
{
	return QModelIndex();
}


bool ExpandDataProxyModel::isRowVirtual(int row) const
{
	if (!dataSetSourceModel())
		return false;

	return row >= dataSetSourceModel()->rowCount();
}

bool ExpandDataProxyModel::isColumnVirtual(int col) const
{
	if (!dataSetSourceModel())
		return false;

	return col >= dataSetSourceModel()->columnCount();
}


void ExpandDataProxyModel::removeRows(int start, int count)
{
	if (!dataSetSourceModel() || count <= 0 || start < 0 || start >= dataSetSourceModel()->rowCount())
		return;

	if (start + count > dataSetSourceModel()->rowCount())
		count = dataSetSourceModel()->rowCount() - start;

	undoStack()->pushCommand(new RemoveRowsCommand(dataSetSourceModel(), start, count));
}

void ExpandDataProxyModel::removeRowGroups(std::vector<std::pair<int, int> > groups)
{
	int rows = 0;
	for(const auto & startCount : groups)
		rows += startCount.second; 
	
	if(!rows)
		return;
	
	undoStack()->startMacro(tr("Remove %1 rows").arg(rows));
	for(const auto & startCount : groups)
		undoStack()->pushCommand(new RemoveRowsCommand(dataSetSourceModel(), startCount.first, startCount.second));
	
	undoStack()->endMacro();
}

void ExpandDataProxyModel::removeColumns(int start, int count)
{
	if (!dataSetSourceModel() || count <= 0 || start < 0 || start >= dataSetSourceModel()->columnCount())
		return;

	if (start + count > dataSetSourceModel()->columnCount())
		count = dataSetSourceModel()->columnCount() - start;
	
	undoStack()->pushCommand(new RemoveColumnsCommand(dataSetSourceModel(), start, count));
}

void ExpandDataProxyModel::removeColumnGroups(std::vector<std::pair<int, int> > groups)
{
	int cols = 0;
	for(const auto & startCount : groups)
		cols += startCount.second; 
	
	if(!cols)
		return;
	
	undoStack()->startMacro(tr("Remove %1 columns").arg(cols));
	for(const auto & startCount : groups)
		new RemoveColumnsCommand(dataSetSourceModel(), startCount.first, startCount.second);
	
	undoStack()->endMacro();
}

void ExpandDataProxyModel::insertRows(int row, int count)
{
	if (!dataSetSourceModel())
		return;

	undoStack()->pushCommand(new InsertRowsCommand(dataSetSourceModel(), row, count));
}


void ExpandDataProxyModel::insertColumns(int col, int count)
{
	if (!dataSetSourceModel())
		return;

	undoStack()->pushCommand(new InsertColumnsCommand(dataSetSourceModel(), col, count));
}


void ExpandDataProxyModel::insertColumn(int col, bool computed, bool R)
{
	if (!dataSetSourceModel())
		return;

	QMap<QString, QVariant> props;
	if (computed)
		props["computed"] = int(R ? computedColumnType::rCode : computedColumnType::constructorCode);
	undoStack()->pushCommand(new InsertColumnCommand(dataSetSourceModel(), col, props));
}

void ExpandDataProxyModel::resize(int row, int col, bool onlyExpand, const QString& undoText)
{
	if (!dataSetSourceModel() || row < 0 || col < 0)
		return;

	if (onlyExpand)
	{
		if (col < dataSetSourceModel()->columnCount() && row < dataSetSourceModel()->rowCount()) return;
	}
	else
	{
		if (col == (dataSetSourceModel()->columnCount() - 1) && row == (dataSetSourceModel()->rowCount() - 1)) return;
	}

	undoStack()->startMacro(undoText);

	if(col >= dataSetSourceModel()->columnCount())
	{	
		int colNr = dataSetSourceModel()->columnCount(),
			colC  = 1 + col - colNr;
		
		if(colC > 0)
			insertColumns(colNr, colC);
	}
	else if (!onlyExpand && col < (dataSetSourceModel()->columnCount() - 1))
		removeColumns(col + 1, dataSetSourceModel()->columnCount() - col - 1);

	if(row >= dataSetSourceModel()->rowCount())
	{	
		int rowNr = dataSetSourceModel()->rowCount(),
			rowC  = 1 + row - rowNr;
		
		if(rowC > 0)
			insertRows(rowNr, rowC);
	}
	else if (!onlyExpand && row < (dataSetSourceModel()->rowCount() - 1))
		removeRows(row + 1, dataSetSourceModel()->rowCount() - row - 1);

	if (!undoText.isEmpty())
		undoStack()->endMacro();
}

bool ExpandDataProxyModel::useUndoStack() const
{
	return dynamic_cast<DataSetTableModel*>(dataSetSourceModel());
}

bool ExpandDataProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!dataSetSourceModel() || index.row() < 0 || index.column() < 0)
		return false;
	
	if(!useUndoStack())
	{
		return dataSetSourceModel()->setData(dataSetSourceModel()->index(index.row(), index.column()), value, role);	
	}

	resize(index.row(), index.column());
	undoStack()->endMacro(new SetDataCommand(dataSetSourceModel(), index.row(), index.column(), value, role));
	return true;
}

void ExpandDataProxyModel::pasteSpreadsheet(int row, int col, const std::vector<std::vector<QString>> & values, const std::vector<std::vector<QString>> & labels, const QStringList & colNames, const std::vector<boolvec> & selected)
{
	if (!dataSetSourceModel() || row < 0 || col < 0 || values.size() == 0 || values[0].size() == 0 )
		return;

	resize(row + values[0].size() - 1, col + values.size() - 1);
	undoStack()->endMacro(new PasteSpreadsheetCommand(dataSetSourceModel(), row, col, values, labels, selected, colNames));
}

stringset ExpandDataProxyModel::columnIndexesToNames(intset columnIndexes)
{
	stringset colNames;

	for(int i : columnIndexes)
		colNames.insert(fq(headerData(i, Qt::Horizontal, int(dataPkgRoles::name)).toString()));

	return colNames;
}

int ExpandDataProxyModel::setColumnType(intset columnIndexes, int columnType)
{

	undoStack()->pushCommand(new SetColumnTypeCommand(dataSetSourceModel(), columnIndexesToNames(columnIndexes), columnType));

	return columnType; //it always works
}

void ExpandDataProxyModel::columnReverseValues(intset columnIndexes)
{
	undoStack()->pushCommand(new ColumnReverseValuesCommand(dataSetSourceModel(), columnIndexesToNames(columnIndexes)));
}

void ExpandDataProxyModel::columnautoSortByValues(intset columnIndexes)
{
	undoStack()->pushCommand(new ColumnToggleAutoSortByValuesCommand(dataSetSourceModel(), columnIndexesToNames(columnIndexes)));
}

void ExpandDataProxyModel::copyColumns(int startCol, const std::vector<Json::Value>& copiedColumns)
{
	if (!dataSetSourceModel() || startCol < 0 || copiedColumns.size() == 0)
		return;

	resize(0, startCol + copiedColumns.size() - 1);
	undoStack()->endMacro(new CopyColumnsCommand(dataSetSourceModel(), startCol, copiedColumns));
}

Json::Value ExpandDataProxyModel::serializedColumn(int col)
{
	if (col < dataSetSourceModel()->columnCount())
	{
		
		return dataSetSourceModel()->column(col)->serialize();
	}

	return Json::nullValue;
}

DataSet	* ExpandDataProxyModel::dataSetSourceModel() const 
{ 
	DataSetTableModel * table = qobject_cast<DataSetTableModel*>(sourceModel()); 
	
	if(table)
		return table->dataSetSourceModel();
	
	return nullptr;
}
