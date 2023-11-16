#ifndef DATASETPACKAGESUBNODEMODEL_H
#define DATASETPACKAGESUBNODEMODEL_H

#include <QAbstractTableModel>
#include "datasetbasenode.h"

class DataSetPackage;

class DataSetPackageSubNodeModel : public QAbstractTableModel
{
	Q_OBJECT

	friend DataSetPackage;

public:
	DataSetPackageSubNodeModel(const QString & whatAmI, DataSetBaseNode * node = nullptr);

	QModelIndex				mapToSource(	const QModelIndex & proxyIndex)													const;
	QModelIndex				mapFromSource(	const QModelIndex & sourceIndex)												const;


	QHash<int, QByteArray>	roleNames()																						const	override;
	int						rowCount(		const QModelIndex &parent = QModelIndex())										const	override;
	int						columnCount(	const QModelIndex &parent = QModelIndex())										const	override;
	QVariant				data(			const QModelIndex &index, int role = Qt::DisplayRole)							const	override;
	QVariant				headerData(		int section, Qt::Orientation orientation, int role = Qt::DisplayRole )			const	override;
	bool					setData(		const QModelIndex &index, const QVariant &value, int role)								override;
	Qt::ItemFlags			flags(			const QModelIndex &index)														const	override;
	bool					insertRows(		int row,		int count,  const QModelIndex & aparent = QModelIndex())				override;
	bool					insertColumns(	int column,		int count,  const QModelIndex & aparent = QModelIndex())				override;
	bool					removeRows(		int row,		int count,  const QModelIndex & aparent = QModelIndex())				override;
	bool					removeColumns(	int column,		int count,  const QModelIndex & aparent = QModelIndex())				override;


	QString				insertColumnSpecial(int column, const QMap<QString, QVariant>& props);
	QString				appendColumnSpecial(			const QMap<QString, QVariant>& props);

	void				selectNode(DataSetBaseNode * node);
	DataSetBaseNode	*	node()	const { return _node; }
	DataSetPackage	*	pkg()	const;
	
signals:
	void				nodeChanged();
	
public slots:
	void				modelWasReset();
	void				dataChangedHandler(				const QModelIndex &topLeft, const QModelIndex &bottomRight,  const QList<int> &roles = QList<int>());
	
	void				rowsAboutToBeInsertedHandler(	const QModelIndex &parent, int first, int last);
	void				rowsInsertedHandler(			const QModelIndex &parent, int first, int last);
	
	void				rowsAboutToBeRemovedHandler(	const QModelIndex &parent, int first, int last);
	void				rowsRemovedHandler(				const QModelIndex &parent, int first, int last);
	
	void				columnsAboutToBeInsertedHandler(const QModelIndex &parent, int first, int last);
	void				columnsInsertedHandler(			const QModelIndex &parent, int first, int last);
	
	void				columnsAboutToBeRemovedHandler(	const QModelIndex &parent, int first, int last);
	void				columnsRemovedHandler(			const QModelIndex &parent, int first, int last);

private:
	DataSetBaseNode		*	_node		= nullptr;
	const QString			_whatAmI	= "?";
};

#endif // DATASETPACKAGESUBNODEMODEL_H
