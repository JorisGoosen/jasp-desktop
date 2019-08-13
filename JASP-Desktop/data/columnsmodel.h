#ifndef COLUMNSMODEL_H
#define COLUMNSMODEL_H

#include <QTransposeProxyModel>
#include "datasettablemodel.h"
#include "common.h"

///Surprisingly the columns are laid out as rows ;-)
class ColumnsModel  : public QTransposeProxyModel
{
	Q_OBJECT
public:
	enum ColumnsModelRoles {
		NameRole = Qt::UserRole + 1,
		TypeRole,
		IconSourceRole,
		ToolTipRole
	 };

	ColumnsModel(DataSetTableModel * tableModel) : QTransposeProxyModel(tableModel), _tableModel(tableModel)
	{
		setSourceModel(_tableModel);
		connect(_tableModel, &DataSetTableModel::headerDataChanged, this, &ColumnsModel::onHeaderDataChanged);
	}

	//int rowCount(const QModelIndex &parent = QModelIndex())				const override { return _dataSet == NULL ? 0 : _dataSet->columnCount();  }
	//int columnCount(const QModelIndex &parent = QModelIndex())			const override { const static int roleNamesCount = roleNames().size(); return roleNamesCount; }
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames()									const override;

private slots:
	void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);

//public slots:
	//void refresh() { beginResetModel(); endResetModel(); }
	//void refreshColumn(Column * column);

	//void datasetHeaderDataChanged(Qt::Orientation orientation, int first, int last);

private:
	DataSetTableModel * _tableModel = nullptr;
};



#endif // COLUMNSMODEL_H
