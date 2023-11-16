#include "datasetpackagesubnodemodel.h"
#include "datasetpackage.h"
#include "log.h"

DataSetPackageSubNodeModel::DataSetPackageSubNodeModel(const QString & whatAmI, DataSetBaseNode * node)
	: QAbstractTableModel(DataSetPackage::pkg()), _node(node), _whatAmI(whatAmI)
{
	connect(pkg(),	&DataSetPackage::modelReset,				this,	&DataSetPackageSubNodeModel::modelWasReset);
	connect(pkg(),	&DataSetPackage::dataChanged,				this,	&DataSetPackageSubNodeModel::dataChangedHandler);
	
	connect(pkg(),	&DataSetPackage::rowsAboutToBeInserted,		this,	&DataSetPackageSubNodeModel::rowsAboutToBeInsertedHandler);
	connect(pkg(),	&DataSetPackage::rowsInserted,				this,	&DataSetPackageSubNodeModel::rowsInsertedHandler);
	
	connect(pkg(),	&DataSetPackage::rowsAboutToBeRemoved,		this,	&DataSetPackageSubNodeModel::rowsAboutToBeRemovedHandler);
	connect(pkg(),	&DataSetPackage::rowsRemoved,				this,	&DataSetPackageSubNodeModel::rowsRemovedHandler);
	
	connect(pkg(),	&DataSetPackage::columnsAboutToBeInserted,	this,	&DataSetPackageSubNodeModel::columnsAboutToBeInsertedHandler);
	connect(pkg(),	&DataSetPackage::columnsInserted,			this,	&DataSetPackageSubNodeModel::columnsInsertedHandler);
	
	connect(pkg(),	&DataSetPackage::columnsAboutToBeRemoved,	this,	&DataSetPackageSubNodeModel::columnsAboutToBeRemovedHandler);
	connect(pkg(),	&DataSetPackage::columnsRemoved,			this,	&DataSetPackageSubNodeModel::columnsRemovedHandler);
	

}


QModelIndex	DataSetPackageSubNodeModel::mapToSource(const QModelIndex & proxyIndex)	const
{
	if(!_node)
		return QModelIndex();
	
	QModelIndex sourceParent = DataSetPackage::pkg()->indexForSubNode(_node);

	if(!proxyIndex.isValid())
		return sourceParent;

	return DataSetPackage::pkg()->index(proxyIndex.row(), proxyIndex.column(), sourceParent);
}

QModelIndex	DataSetPackageSubNodeModel::mapFromSource(const QModelIndex &sourceIndex) const
{
	if(!_node)
		return QModelIndex();
	
	QModelIndex sourceParentGiven = sourceIndex.parent(),
				sourceParentKnown = DataSetPackage::pkg()->indexForSubNode(_node);

	if(	sourceParentGiven != sourceParentKnown)
		return QModelIndex();

	return createIndex(sourceIndex.row(), sourceIndex.column(), nullptr);
}

QHash<int, QByteArray> DataSetPackageSubNodeModel::roleNames() const
{
	return DataSetPackage::pkg()->roleNames();
}

int DataSetPackageSubNodeModel::rowCount(const QModelIndex & parent) const
{
	int row = !_node ? 0 :DataSetPackage::pkg()->rowCount(mapToSource(parent));
	//Log::log() << "DataSetPackageSubNodeModel("<< _whatAmI.toStdString() << ")::rowCount(" << ( _node ? dataSetBaseNodeTypeToString(_node->nodeType()) : "no node") << ") = " << row << std::endl;
	return row;
}

int DataSetPackageSubNodeModel::columnCount(const QModelIndex & parent) const
{
	int col = !_node ? 0 : DataSetPackage::pkg()->columnCount(mapToSource(parent));
	//Log::log() << "DataSetPackageSubNodeModel("<< _whatAmI.toStdString() << ")::columnCount(" << ( _node ? dataSetBaseNodeTypeToString(_node->nodeType()) : "no node")  << ") = " << col << std::endl;
	return col;
}

QVariant DataSetPackageSubNodeModel::data(const QModelIndex &index, int role) const
{
	return !_node ? QVariant() : DataSetPackage::pkg()->data(mapToSource(index), role);
}

QVariant DataSetPackageSubNodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return DataSetPackage::pkg()->headerDataForNode(_node, section, orientation, role);
}

bool DataSetPackageSubNodeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	return _node && DataSetPackage::pkg()->setData(mapToSource(index), value, role);
}

Qt::ItemFlags DataSetPackageSubNodeModel::flags(const QModelIndex &index) const
{
	return !_node ? Qt::ItemFlags() : DataSetPackage::pkg()->flags(mapToSource(index));
}

bool DataSetPackageSubNodeModel::insertRows(int row, int count, const QModelIndex &aparent)
{
	return _node && DataSetPackage::pkg()->insertRows(row, count, mapToSource(aparent));
}

bool DataSetPackageSubNodeModel::insertColumns(int column, int count, const QModelIndex &aparent)
{
	return _node && DataSetPackage::pkg()->insertColumns(column, count, mapToSource(aparent));
}

bool DataSetPackageSubNodeModel::removeRows(int row, int count, const QModelIndex &aparent)
{
	return _node && DataSetPackage::pkg()->removeRows(row, count, mapToSource(aparent));
}

bool DataSetPackageSubNodeModel::removeColumns(int column, int count, const QModelIndex &aparent)
{
	return _node && DataSetPackage::pkg()->removeColumns(column, count, mapToSource(aparent));
}


QString DataSetPackageSubNodeModel::insertColumnSpecial(int column, const QMap<QString, QVariant>& props)
{
	int sourceColumn = column > columnCount() ? columnCount() : column;
	sourceColumn = mapToSource(index(0, sourceColumn)).column();

	return DataSetPackage::pkg()->insertColumnSpecial(sourceColumn == -1 ? pkg()->columnCount() : sourceColumn, props);
}

QString DataSetPackageSubNodeModel::appendColumnSpecial(const QMap<QString, QVariant>& props)
{
	return DataSetPackage::pkg()->appendColumnSpecial(props);
}

void DataSetPackageSubNodeModel::modelWasReset()
{
	//The model was reset, which means _node might no longer exist!
	if(!DataSetPackage::pkg()->dataSetBaseNodeStillExists(_node))
		selectNode(nullptr);

	beginResetModel();
	endResetModel();
}

void DataSetPackageSubNodeModel::selectNode(DataSetBaseNode * node)
{
	if (_node == node)
		return;

	beginResetModel();
	_node = node;
	emit nodeChanged();
	endResetModel();
}

DataSetPackage *DataSetPackageSubNodeModel::pkg() const { return DataSetPackage::pkg(); }

void DataSetPackageSubNodeModel::dataChangedHandler(const QModelIndex &topLeft, const QModelIndex &bottomRight,  const QList<int> &roles)
{
	QModelIndex tl(mapFromSource(topLeft)), br(mapFromSource(bottomRight));
	
	if(tl.isValid() && br.isValid())
		emit dataChanged(tl, br, roles);
}

//avoid unnecessary hickups in the downstream models by only ever mentioning relevant things

#define HANDLERBEGIN(NAME,CALLEE)															\
void DataSetPackageSubNodeModel:: NAME (const QModelIndex &parent, int first, int last)		\
{																							\
		QModelIndex p(mapFromSource(parent));												\
																							\
		if(p.isValid())																		\
			CALLEE (p, first, last);														\
}

#define HANDLEREND(NAME,CALLEE)																\
void DataSetPackageSubNodeModel:: NAME (const QModelIndex &parent, int first, int last)		\
{																							\
		QModelIndex p(mapFromSource(parent));												\
																							\
		if(p.isValid())																		\
			CALLEE (first, 1+last-first, p);												\
}

HANDLERBEGIN(	rowsAboutToBeInsertedHandler,		beginInsertRows		)
HANDLEREND(		rowsInsertedHandler,				insertRows			)
HANDLERBEGIN(	rowsAboutToBeRemovedHandler,		beginRemoveRows		)
HANDLEREND(		rowsRemovedHandler,					removeRows			)
HANDLERBEGIN(	columnsAboutToBeInsertedHandler,	beginInsertColumns	)
HANDLEREND(		columnsInsertedHandler,				insertColumns		)
HANDLERBEGIN(	columnsAboutToBeRemovedHandler,		beginRemoveColumns	)
HANDLEREND(		columnsRemovedHandler,				removeColumns		)

