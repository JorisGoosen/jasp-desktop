#ifndef LEVELSTABLEMODEL_H
#define LEVELSTABLEMODEL_H

#include "common.h"
#include "data/datasettableproxy.h"

class LevelsTableModel : public DataSetTableProxy
{
	Q_OBJECT
	Q_PROPERTY(int	filteredOut		READ filteredOut									NOTIFY filteredOutChanged)
	Q_PROPERTY(int	chosenColumn	READ proxyParentColumn	WRITE setProxyParentColumn	NOTIFY proxyParentColumnChanged)
	Q_PROPERTY(bool visible			READ visible			WRITE setVisible			NOTIFY visibleChanged)

public:
	LevelsTableModel(DataSetPackage * package);
	~LevelsTableModel()	{ }

	/*void setColumn(Column *column);
	Q_INVOKABLE void clearColumn();

	int						rowCount(const QModelIndex &parent = QModelIndex())						const	override;
	int						columnCount(const QModelIndex &parent = QModelIndex())					const	override;
	QVariant				data(const QModelIndex &index, int role)								const	override;
	QVariant				headerData(int section, Qt::Orientation orientation, int role)			const	override;
	Qt::ItemFlags			flags(const QModelIndex &index)											const	override;
	QHash<int, QByteArray>	roleNames()																const	override;*/

	bool					setData(const QModelIndex & index, const QVariant & value, int role)			override;

	void moveUp(QModelIndexList &selection);
	void moveDown(QModelIndexList &selection);

	Q_INVOKABLE void reverse();

	Q_INVOKABLE void moveUpFromQML(QVariantList selection)		{ QModelIndexList List = convertQVariantList_to_QModelIndexList(selection); moveUp(List); }
	Q_INVOKABLE void moveDownFromQML(QVariantList selection)	{ QModelIndexList List = convertQVariantList_to_QModelIndexList(selection); moveDown(List); }

	QModelIndexList convertQVariantList_to_QModelIndexList(QVariantList selection);

	Q_INVOKABLE void resetFilterAllows();

	bool visible()		const {	return _visible; }
	int filteredOut()	const;

public slots:
	void filteredOutChangedHandler(int col);

	void setVisible(bool visible);

signals:
	void visibleChanged(bool visible);
	void filteredOutChanged();

private:
	void	_moveRows(QModelIndexList &selection, bool up = true);

private:
	bool	_visible		= false;

};

#endif // LEVELSTABLEMODEL_H
