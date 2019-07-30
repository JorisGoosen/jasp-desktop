#include "datasettableproxy.h"

DataSetTableProxy::DataSetTableProxy(DataSetPackage * package, parIdxType proxyType)
	: QSortFilterProxyModel(package),
	  _package(package),
	  _proxyType(proxyType)
{
	setSourceModel(_package);
}

QModelIndex	DataSetTableProxy::mapToSource(const QModelIndex &proxyIndex)	const
{
	if(!proxyIndex.isValid())
		return _package->parentModelForType(_proxyType);

	return _package->index(proxyIndex.row(), proxyIndex.column(), _package->parentModelForType(_proxyType));
}

QModelIndex	DataSetTableProxy::mapFromSource(const QModelIndex &sourceIndex) const
{
	if(_package->parentIndexTypeIs(sourceIndex) == _proxyType || _package->parentIndexTypeIs(sourceIndex.parent()) != _proxyType)
		return QModelIndex();

	return index(sourceIndex.row(), sourceIndex.column(), QModelIndex());
}
