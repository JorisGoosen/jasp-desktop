#ifndef DATASETBASENODE_H
#define DATASETBASENODE_H

#include "dataenums.h"
#include <QAbstractTableModel>

/// Special class to be used as nodes in the overall "data"-structure, also used in QModelIndex pointer in DataSetPackage
/// 
/// This class registers the DataSetBaseNode parent it has and announces itself as child to it.
/// QObjects of course do this too, but we cant use those in the engine/R so here we are
/// It also handles updating revisions to make sure all updates and changes to the data are registered.
/// The subclasses do need to override incRevision() if the changed value should go into the database
/// 
/// This structure of children and parents is used to both mirror the database as the necessary structure for all
/// derived classes and is the underlying treemodel for DataSetPackage
class DataSetBaseNode : public QAbstractTableModel
{
	Q_OBJECT
public:
			typedef std::set<DataSetBaseNode*> NodeSet;
	
									DataSetBaseNode(dataSetBaseNodeType typeNode, QObject * parent = nullptr);
									~DataSetBaseNode();
	
			dataSetBaseNodeType		nodeType() const { return _type; }
	
			void					registerChild(	DataSetBaseNode * child);
			void					unregisterChild(DataSetBaseNode * child);
			bool					nodeStillExists(DataSetBaseNode * node)		const;
	
			DataSetBaseNode		*	parent() const { return _parent; }
	
	virtual	void					incRevision();	///< Any overrides MUST call checkForChanges()
			QHash<int, QByteArray>	roleNames() const override;
	
			int						revision() { return _revision; }
			int						nestedRevision();

protected:
			void					checkForChanges();
			
			dataSetBaseNodeType		_type		= dataSetBaseNodeType::unknown;
			DataSetBaseNode		*	_parent		= nullptr;
			NodeSet					_children;
			int						_revision	= 1; ///< We use revision both inside the database to track whether a node should be reloaded. but also to set packageModified in DataSetPackage on changes
			
signals:
			void					somethingModified();
			
private:
			int						_previousNestedRevision;			
};

#endif // DATASETBASENODE_H
