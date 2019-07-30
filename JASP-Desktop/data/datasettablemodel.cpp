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

#include "datasettablemodel.h"

DataSetTableModel::DataSetTableModel(DataSetPackage * package)
	: QAbstractProxyModel(package), _package(package)
{
	setSourceModel(_package);
	connect(_package, &DataSetPackage::columnsFilteredCountChanged, this, &DataSetTableModel::columnsFilteredCountChanged	);
	connect(_package, &DataSetPackage::modelAboutToBeReset,			this, &DataSetTableModel::modelAboutToBeResetHandler	);
	connect(_package, &DataSetPackage::modelReset,					this, &DataSetTableModel::modelResetHandler				);
}

QModelIndex	DataSetTableModel::mapToSource(const QModelIndex &proxyIndex)	const
{
	if(!proxyIndex.isValid())
		return _package->parentModelForType(parIdxType::data);

	return _package->index(proxyIndex.row(), proxyIndex.column(), _package->parentModelForType(parIdxType::data));
}

QModelIndex	DataSetTableModel::mapFromSource(const QModelIndex &sourceIndex) const
{
	if(_package->parentIndexTypeIs(sourceIndex) == parIdxType::data || _package->parentIndexTypeIs(sourceIndex.parent()) != parIdxType::data)
		return QModelIndex();

	return index(sourceIndex.row(), sourceIndex.column(), QModelIndex());
}

QModelIndex	DataSetTableModel::parent(const QModelIndex & index) const
{
	return QModelIndex();
}

QModelIndex	DataSetTableModel::index(int row, int column, const QModelIndex &parent) const
{
	return createIndex(row, column, nullptr);
}

int	DataSetTableModel::rowCount(const QModelIndex &parent) const
{
	int rowCount = _package->rowCount(mapToSource(parent));
	return rowCount;
}

int	DataSetTableModel::columnCount(const QModelIndex &parent) const
{
	int colCount = _package->columnCount(mapToSource(parent));;
	return colCount;
}

QVariant DataSetTableModel::data(const QModelIndex & index, int role) const
{
	QVariant returnThis = _package->data(mapToSource(index), role);

	return returnThis;
}
