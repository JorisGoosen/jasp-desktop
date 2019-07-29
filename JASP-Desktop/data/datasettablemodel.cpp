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


QModelIndex	DataSetTableModel::mapToSource(const QModelIndex &proxyIndex)	const
{
	if(!proxyIndex.isValid())
		return _package->parentModelForType(parIdxType::data);

	return _package->index(proxyIndex.row(), proxyIndex.column(), _package->parentModelForType(parIdxType::data));
}

QModelIndex	DataSetTableModel::mapFromSource(const QModelIndex &sourceIndex) const
{
	if(_package->parentModelIndexIs(sourceIndex) == parIdxType::data)
		return QModelIndex();

	return index(sourceIndex.row(), sourceIndex.column(), QModelIndex());
}

/*
int	DataSetTableModel::rowCount(const QModelIndex &parent) const
{
	std::cout << "rowCount of parent " << parIdxTypeToString(_package->parentModelIndexIs(parent)) << std::flush;
	int rowC = _package->rowCount(mapToSource(_package->parentModelForType

}
int	DataSetTableModel::columnCount(const QModelIndex &parent = QModelIndex())								const	override;*/
