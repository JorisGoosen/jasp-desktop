#include "dataenums.h"
#include "datasetbasenode.h"
#include <QThread>
#include <QGuiApplication>

DataSetBaseNode::DataSetBaseNode(dataSetBaseNodeType typeNode, QObject * parent) 
	: QAbstractTableModel(nullptr), _type(typeNode), _parent(dynamic_cast<DataSetBaseNode *>(parent))
{
	if(_parent)
		_parent->registerChild(this);
	
	if(QGuiApplication::instance())
		this->moveToThread(QGuiApplication::instance()->thread());
	
	setParent(parent);
}

DataSetBaseNode::~DataSetBaseNode()
{
	if(_parent)
		_parent->unregisterChild(this);
	_parent = nullptr;
}

void DataSetBaseNode::registerChild(DataSetBaseNode *child)
{
	_children.insert(child);
}

void DataSetBaseNode::unregisterChild(DataSetBaseNode *child)
{
	_children.erase(child);
}

bool DataSetBaseNode::nodeStillExists(DataSetBaseNode *node) const
{
	if(node == this)
		return true;
	
	for(DataSetBaseNode * child : _children)
		if(child->nodeStillExists(node))
			return true;

	return false;
}

void DataSetBaseNode::incRevision()
{
	_revision++;
	checkForChanges();
}

int DataSetBaseNode::nestedRevision()
{
	int rev = _revision;
	
	for(DataSetBaseNode * child : _children)
		rev *= child->nestedRevision();
	
	return rev;
}

void DataSetBaseNode::checkForChanges()
{
	if(_parent)
		_parent->checkForChanges();
	else
	{
		int nested = nestedRevision();
		
		if(nested != _previousNestedRevision)
			emit somethingModified();
		
		_previousNestedRevision = nested;
	}
}

QHash<int, QByteArray> DataSetBaseNode::roleNames() const
{
	static bool						set = false;
	static QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();

	if(!set)
	{
		for(const auto & enumString : dataPkgRolesToStringMap())
			roles[int(enumString.first)] = QString::fromStdString(enumString.second).toUtf8();

		set = true;
	}

	return roles;
}
