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

#include "datasettableproxy.h"


class DataSetTableModel : public DataSetTableProxy
{
	Q_OBJECT
	Q_PROPERTY(int	columnsFilteredCount	READ columnsFilteredCount							NOTIFY columnsFilteredCountChanged)
	Q_PROPERTY(bool showInactive			READ showInactive			WRITE setShowInactive	NOTIFY showInactiveChanged)

public:
	explicit				DataSetTableModel(DataSetPackage * package);

	bool					filterAcceptsRows(int source_row, const QModelIndex & source_parent)	const;

				int			columnsFilteredCount()					const				{ return _package->columnsFilteredCount(); }
	Q_INVOKABLE bool		isColumnNameFree(QString name)								{ return _package->isColumnNameFree(name);								}
	Q_INVOKABLE bool		getRowFilter(int row)					const				{ return _package->getRowFilter(row);									}
	Q_INVOKABLE	QVariant	columnTitle(int column)					const				{ return _package->columnTitle(column);									}
	Q_INVOKABLE QVariant	columnIcon(int column)					const				{ return _package->columnIcon(column);									}
	Q_INVOKABLE QVariant	getColumnTypesWithCorrespondingIcon()	const				{ return _package->getColumnTypesWithCorrespondingIcon();				}
	Q_INVOKABLE bool		columnHasFilter(int column)				const				{ return _package->columnHasFilter(column);								}
	Q_INVOKABLE bool		columnUsedInEasyFilter(int column)		const				{ return _package->columnUsedInEasyFilter(column);						}
	Q_INVOKABLE void		resetAllFilters()											{		 _package->resetAllFilters();									}
	Q_INVOKABLE int			setColumnTypeFromQML(int columnIndex, int newColumnType)	{ return _package->setColumnTypeFromQML(columnIndex, newColumnType);	}

	Column::ColumnType		columnType(size_t column)					const				{ return _package->columnType(column);								}
	std::string				columnName(size_t col)						const				{ return _package->getColumnName(col);								}

				bool		showInactive()															const				{ return _showInactive;	}

signals:
				void		columnsFilteredCountChanged();
				void		showInactiveChanged(bool showInactive);

public slots:
				void		setShowInactive(bool showInactive);

private:
	bool					_showInactive	= false;
};

#endif // DATASETTABLEMODEL_H
