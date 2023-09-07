#ifndef DATASETPACKAGESUBNODEMODEL_H
#define DATASETPACKAGESUBNODEMODEL_H

#include <QIdentityProxyModel>
#include "datasetbasenode.h"

class DataSetPackage;

class DataSetPackageSubNodeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	DataSetPackageSubNodeModel(const QString & whatAmI, DataSetBaseNode * node = nullptr);

	QModelIndex			mapToSource(	const QModelIndex & proxyIndex)				const;
	QModelIndex			mapFromSource(	const QModelIndex & sourceIndex)			const;
	int					rowCount(		const QModelIndex & parent = QModelIndex())	const	override;
	int					columnCount(	const QModelIndex & parent = QModelIndex())	const	override;

	QString				insertColumnSpecial(int column, bool computed, bool R);
	QString				appendColumnSpecial(			bool computed, bool R);

	
	void				selectNode(DataSetBaseNode * node);
	DataSetBaseNode	*	node()	const { return _node; }
	DataSetPackage	*	pkg()	const;
	
signals:
	void				nodeChanged();
	
public slots:
	void				modelWasReset();
	void				dataChangedHandler(const QModelIndex &topLeft, const QModelIndex &bottomRight,  const QList<int> &roles = QList<int>());
	
	void				rowsAboutToBeInsertedHandler(const QModelIndex &parent, int first, int last);
	void				rowsInsertedHandler(const QModelIndex &parent, int first, int last);
	
	void				rowsAboutToBeRemovedHandler(const QModelIndex &parent, int first, int last);
	void				rowsRemovedHandler(const QModelIndex &parent, int first, int last);
	
	void				columnsAboutToBeInsertedHandler(const QModelIndex &parent, int first, int last);
	void				columnsInsertedHandler(const QModelIndex &parent, int first, int last);
	
	void				columnsAboutToBeRemovedHandler(const QModelIndex &parent, int first, int last);
	void				columnsRemovedHandler(const QModelIndex &parent, int first, int last);

private:
	DataSetBaseNode		*	_node		= nullptr;
	const QString			_whatAmI	= "?";
};

#endif // DATASETPACKAGESUBNODEMODEL_H
