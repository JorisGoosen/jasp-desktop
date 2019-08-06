#include "datasettableproxy.h"

DataSetTableProxy::DataSetTableProxy(DataSetPackage * package, parIdxType proxyType)
	: QSortFilterProxyModel(package),
	  _package(package),
	  _proxyType(proxyType)
{
	setSourceModel(_package);

	connect(_package, &DataSetPackage::columnsRemoved,	this, &DataSetTableProxy::columnsWereRemoved);
	connect(_package, &DataSetPackage::modelReset,		this, &DataSetTableProxy::modelWasReset		);
}

QModelIndex	DataSetTableProxy::mapToSource(const QModelIndex &proxyIndex)	const
{
	if(!proxyIndex.isValid())
		return _package->parentModelForType(_proxyType, _proxyParentColumn);

	return _package->index(proxyIndex.row(), proxyIndex.column(), _package->parentModelForType(_proxyType, _proxyParentColumn));
}

QModelIndex	DataSetTableProxy::mapFromSource(const QModelIndex &sourceIndex) const
{
	QModelIndex sourceParent = sourceIndex.parent();

	if(_package->parentIndexTypeIs(sourceIndex) == _proxyType || _package->parentIndexTypeIs(sourceParent) != _proxyType || sourceParent.column() != _proxyParentColumn)
		return QModelIndex();

	return index(sourceIndex.row(), sourceIndex.column(), QModelIndex());
}

void DataSetTableProxy::setProxyParentColumn(int proxyParentColumn)
{
	if (_proxyParentColumn == proxyParentColumn)
		return;

	beginResetModel();
	_proxyParentColumn = proxyParentColumn;
	emit proxyParentColumnChanged();
	endResetModel();
}


void DataSetTableProxy::modelWasReset()
{
	if(_proxyParentColumn >= columnCount())
		setProxyParentColumn(std::max(0, columnCount() - 1));
}
