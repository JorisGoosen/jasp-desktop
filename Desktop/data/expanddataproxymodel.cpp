#include "expanddataproxymodel.h"
#include "datasettablemodel.h"
#include "dataenums.h"

ExpandDataProxyModel::ExpandDataProxyModel(QObject *parent)
	: QIdentityProxyModel{parent}
{
	connect(undoStack(), &QUndoStack::indexChanged, this, &ExpandDataProxyModel::undoChanged) ;
}

int ExpandDataProxyModel::rowCount(const QModelIndex &) const
{
	if (!sourceModel())
		return 0;
	return sourceModel()->rowCount() + (_expandDataSet ? EXTRA_ROWS : 0);
}

int ExpandDataProxyModel::columnCount(const QModelIndex &) const
{
	if (!sourceModel())
		return 0;
	return sourceModel()->columnCount() + (_expandDataSet ? EXTRA_COLS : 0);
}

QVariant ExpandDataProxyModel::data(const QModelIndex &indexP, int role) const
{
	if (!sourceModel() || role == -1) // Role not defined
		return QVariant();

	int row		= indexP.row(),
		column	= indexP.column();

	if (column < sourceModel()->columnCount() && row < sourceModel()->rowCount())
		return sourceModel()->data(sourceModel()->index(row, column), role);

	switch(role)
	{
	case int(dataPkgRoles::selected):				return false;
	case int(dataPkgRoles::lines):
	{
		DataSetTableModel * dataSetTable = dynamic_cast<DataSetTableModel *>(sourceModel());

		if (row < sourceModel()->rowCount() && dataSetTable && dataSetTable->showInactive() && !data(index(row,0), int(dataPkgRoles::filter)).toBool())
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
	if (!sourceModel() || role == -1) // Role not defined
		return QVariant();

	if (orientation == Qt::Orientation::Horizontal)
	{
		if (section < sourceModel()->columnCount())
			return sourceModel()->headerData(section, orientation, role);
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
		if (section < sourceModel()->rowCount())
			return sourceModel()->headerData(section, orientation, role);
		else if (section == 0 && role == int(dataPkgRoles::maxRowHeaderString))
			return "XXXX";
		else
			return  DataSetPackage::pkg()->dataRowCount() + (section - sourceModel()->rowCount()) + 1;
	}

	return QVariant();
}

Qt::ItemFlags ExpandDataProxyModel::flags(const QModelIndex &index) const
{
	if (!sourceModel())
		return Qt::NoItemFlags;

	if (index.column() < sourceModel()->columnCount() && index.row() < sourceModel()->rowCount())
		return sourceModel()->flags(sourceModel()->index(index.row(), index.column()));

	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QModelIndex ExpandDataProxyModel::index(int row, int column, const QModelIndex &) const
{
	if (!sourceModel())
		return QModelIndex();

	return createIndex(row, column);
}

QModelIndex ExpandDataProxyModel::parent(const QModelIndex &index) const
{
	return QModelIndex();
}


bool ExpandDataProxyModel::isRowVirtual(int row) const
{
	if (!sourceModel())
		return false;

	return row >= sourceModel()->rowCount();
}

bool ExpandDataProxyModel::isColumnVirtual(int col) const
{
	if (!sourceModel())
		return false;

	return col >= sourceModel()->columnCount();
}


void ExpandDataProxyModel::removeRows(int start, int count)
{
	if (!sourceModel() || count <= 0 || start < 0 || start >= sourceModel()->rowCount())
		return;

	if (start + count > sourceModel()->rowCount())
		count = sourceModel()->rowCount() - start;

	undoStack()->pushCommand(new RemoveRowsCommand(sourceModel(), start, count));
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
		undoStack()->pushCommand(new RemoveRowsCommand(sourceModel(), startCount.first, startCount.second));
	
	undoStack()->endMacro();
}

void ExpandDataProxyModel::removeColumns(int start, int count)
{
	if (!sourceModel() || count <= 0 || start < 0 || start >= sourceModel()->columnCount())
		return;

	if (start + count > sourceModel()->columnCount())
		count = sourceModel()->columnCount() - start;
	
	undoStack()->pushCommand(new RemoveColumnsCommand(sourceModel(), start, count));
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
		new RemoveColumnsCommand(sourceModel(), startCount.first, startCount.second);
	
	undoStack()->endMacro();
}

void ExpandDataProxyModel::insertRows(int row, int count)
{
	if (!sourceModel())
		return;

	undoStack()->pushCommand(new InsertRowsCommand(sourceModel(), row, count));
}


void ExpandDataProxyModel::insertColumns(int col, int count)
{
	if (!sourceModel())
		return;

	undoStack()->pushCommand(new InsertColumnsCommand(sourceModel(), col, count));
}


void ExpandDataProxyModel::insertColumn(int col, bool computed, bool R)
{
	if (!sourceModel())
		return;

	QMap<QString, QVariant> props;
	if (computed)
		props["computed"] = int(R ? computedColumnType::rCode : computedColumnType::constructorCode);
	undoStack()->pushCommand(new InsertColumnCommand(sourceModel(), col, props));
}

void ExpandDataProxyModel::resize(int row, int col, bool onlyExpand, const QString& undoText)
{
	if (!sourceModel() || row < 0 || col < 0)
		return;

	if (onlyExpand)
	{
		if (col < sourceModel()->columnCount() && row < sourceModel()->rowCount()) return;
	}
	else
	{
		if (col == (sourceModel()->columnCount() - 1) && row == (sourceModel()->rowCount() - 1)) return;
	}

	undoStack()->startMacro(undoText);

	if(col >= sourceModel()->columnCount())
	{	
		int colNr = sourceModel()->columnCount(),
			colC  = 1 + col - colNr;
		
		if(colC > 0)
			insertColumns(colNr, colC);
	}
	else if (!onlyExpand && col < (sourceModel()->columnCount() - 1))
		removeColumns(col + 1, sourceModel()->columnCount() - col - 1);

	if(row >= sourceModel()->rowCount())
	{	
		int rowNr = sourceModel()->rowCount(),
			rowC  = 1 + row - rowNr;
		
		if(rowC > 0)
			insertRows(rowNr, rowC);
	}
	else if (!onlyExpand && row < (sourceModel()->rowCount() - 1))
		removeRows(row + 1, sourceModel()->rowCount() - row - 1);

	if (!undoText.isEmpty())
		undoStack()->endMacro();
}

bool ExpandDataProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!sourceModel() || index.row() < 0 || index.column() < 0)
		return false;

	resize(index.row(), index.column());
	undoStack()->endMacro(new SetDataCommand(sourceModel(), index.row(), index.column(), value, role));
	return true;
}

void ExpandDataProxyModel::pasteSpreadsheet(int row, int col, const std::vector<std::vector<QString>> & values, const std::vector<std::vector<QString>> & labels, const QStringList & colNames, const std::vector<boolvec> & selected)
{
	if (!sourceModel() || row < 0 || col < 0 || values.size() == 0 || values[0].size() == 0 )
		return;

	resize(row + values[0].size() - 1, col + values.size() - 1);
	undoStack()->endMacro(new PasteSpreadsheetCommand(sourceModel(), row, col, values, labels, selected, colNames));
}

int ExpandDataProxyModel::setColumnType(intset columnIndexes, int columnType)
{
	undoStack()->pushCommand(new SetColumnTypeCommand(sourceModel(), columnIndexes, columnType));

	return columnType; //it always works
}

void ExpandDataProxyModel::columnReverseValues(intset columnIndexes)
{
	undoStack()->pushCommand(new ColumnReverseValuesCommand(sourceModel(), columnIndexes));
}

void ExpandDataProxyModel::columnautoSortByValues(intset columnIndexes)
{
    undoStack()->pushCommand(new ColumnToggleAutoSortByValuesCommand(sourceModel(), columnIndexes));
}

void ExpandDataProxyModel::copyColumns(int startCol, const std::vector<Json::Value>& copiedColumns)
{
	if (!sourceModel() || startCol < 0 || copiedColumns.size() == 0)
		return;

	resize(0, startCol + copiedColumns.size() - 1);
	undoStack()->endMacro(new CopyColumnsCommand(sourceModel(), startCol, copiedColumns));
}

Json::Value ExpandDataProxyModel::serializedColumn(int col)
{
	Json::Value result;
	if (col < sourceModel()->columnCount())
	{
		QString colName = sourceModel()->headerData(col, Qt::Orientation::Horizontal).toString();
		if (!colName.isEmpty())
			result = DataSetPackage::pkg()->serializeColumn(colName.toStdString());
	}

	return result;
}
