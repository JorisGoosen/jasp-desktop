#include "columnsmodel.h"

QVariant ColumnsModel::data(const QModelIndex &index, int role) const
{
	if(index.row() < 0 || index.row() >= rowCount()) return QVariant();

	switch(role)
	{
	case NameRole:			return _tableModel->columnTitle(index.row());
	case TypeRole:			return "column";
	case IconSourceRole:
		switch(_tableModel->columnType(index.row()))
		{
		case Column::ColumnType::ColumnTypeScale:		return "qrc:/icons/variable-scale.svg";
		case Column::ColumnType::ColumnTypeOrdinal:		return "qrc:/icons/variable-ordinal.svg";
		case Column::ColumnType::ColumnTypeNominal:		return "qrc:/icons/variable-nominal.svg";
		case Column::ColumnType::ColumnTypeNominalText:	return "qrc:/icons/variable-nominal-text.svg";
		default:										return "";
		}
	case ToolTipRole:
	{
		Column::ColumnType colType = _tableModel->columnType(index.row());
		QString usedIn = colType == Column::ColumnType::ColumnTypeScale ? "which can be used in numerical comparisons" : colType == Column::ColumnType::ColumnTypeOrdinal ? "which can only be used in (in)equivalence, greater and lesser than comparisons" : "which can only be used in (in)equivalence comparisons";
		return "The '" + _tableModel->columnTitle(index.row()).toString() + "'-column " + usedIn;
	}
	}

	return QVariant();
}

QHash<int, QByteArray> ColumnsModel::roleNames() const
{
	static const auto roles = QHash<int, QByteArray>{
		{ NameRole,					"columnName"},
		{ TypeRole,					"type"},
		{ IconSourceRole,			"columnIcon"},
		{ ToolTipRole,				"toolTip"}
	};

	return roles;
}

// It is the headerdata from untransposed source
void ColumnsModel::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
	beginResetModel();
	endResetModel();

	//datachanged doesnt seem to work in filterconstructor etc
	//if(orientation == Qt::Horizontal)
	//	emit dataChanged(index(first, 0), index(last, columnCount()), { NameRole, TypeRole, IconSourceRole, ToolTipRole });
}
