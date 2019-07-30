#ifndef DATASETTABLEPROXY_H
#define DATASETTABLEPROXY_H

#include <QSortFilterProxyModel>
#include "datasetpackage.h"

class DataSetTableProxy : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit				DataSetTableProxy(DataSetPackage * package, parIdxType proxyType);

	QModelIndex				mapToSource(	const QModelIndex & proxyIndex)		const	override;
	QModelIndex				mapFromSource(	const QModelIndex & sourceIndex)	const	override;

protected:
	DataSetPackage		*	_package		= nullptr;

private:
	parIdxType				_proxyType		= parIdxType::root;
};

#endif // DATASETTABLEPROXY_H
