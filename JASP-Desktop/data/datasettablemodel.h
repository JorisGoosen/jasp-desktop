//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

#ifndef DATASETTABLEMODEL_H
#define DATASETTABLEMODEL_H

#include <QModelIndex>
#include <QAbstractProxyModel>
#include <QIcon>

#include "common.h"
#include "datasetpackage.h"


class DataSetTableModel : public QAbstractProxyModel
{
	Q_OBJECT
	Q_PROPERTY(int columnsFilteredCount READ columnsFilteredCount NOTIFY columnsFilteredCountChanged)

public:
	explicit						DataSetTableModel(DataSetPackage * package);


	/*QHash<int, QByteArray>	roleNames()																			const	override;


	QVariant				headerData(	int section, Qt::Orientation orientation, int role = Qt::DisplayRole )	const	override;
	bool					setData(	const QModelIndex &index, const QVariant &value, int role)						override;
	Qt::ItemFlags			flags(		const QModelIndex &index)												const	override;*/

	QModelIndex				index(int row, int column, const QModelIndex &parent = QModelIndex())				const	override;
	QVariant				data(			const QModelIndex & index, int role = Qt::DisplayRole)				const	override;
	QModelIndex				parent(			const QModelIndex & index)											const	override;
	int						rowCount(		const QModelIndex & parent = QModelIndex())							const	override;
	int						columnCount(	const QModelIndex & parent = QModelIndex())							const	override;
	QModelIndex				mapToSource(	const QModelIndex & proxyIndex)										const	override;
	QModelIndex				mapFromSource(	const QModelIndex & sourceIndex)									const	override;

	int						columnsFilteredCount() { return _package->columnsFilteredCount(); }

	Q_INVOKABLE bool		isColumnNameFree(QString name)								{ return _package->isColumnNameFree(name);								}
	Q_INVOKABLE bool		getRowFilter(int row)					const				{ return _package->getRowFilter(row);									}
	Q_INVOKABLE	QVariant	columnTitle(int column)					const				{ return _package->columnTitle(column);									}
	Q_INVOKABLE QVariant	columnIcon(int column)					const				{ return _package->columnIcon(column);									}
	Q_INVOKABLE QVariant	getColumnTypesWithCorrespondingIcon()	const				{ return _package->getColumnTypesWithCorrespondingIcon();				}
	Q_INVOKABLE bool		columnHasFilter(int column)				const				{ return _package->columnHasFilter(column);								}
	Q_INVOKABLE bool		columnUsedInEasyFilter(int column)		const				{ return _package->columnUsedInEasyFilter(column);						}
	Q_INVOKABLE void		resetAllFilters()											{		 _package->resetAllFilters();									}
	Q_INVOKABLE int			setColumnTypeFromQML(int columnIndex, int newColumnType)	{ return _package->setColumnTypeFromQML(columnIndex, newColumnType);	}


public slots:
	void modelAboutToBeResetHandler()	{ beginResetModel();	}
	void modelResetHandler()			{ endResetModel();		}

signals:
	void columnsFilteredCountChanged();
	/*void QAbstractItemModel::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = ...)
	void QAbstractItemModel::headerDataChanged(Qt::Orientation orientation, int first, int last)*/


private:
	DataSetPackage				*_package = nullptr;
};

#endif // DATASETTABLEMODEL_H
