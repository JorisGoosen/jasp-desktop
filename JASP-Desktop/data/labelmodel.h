#ifndef LABELMODEL_H
#define LABELMODEL_H


#include "datasettableproxy.h"

class LabelModel : public DataSetTableProxy
{
	Q_OBJECT
	Q_PROPERTY(int	filteredOut		READ filteredOut									NOTIFY filteredOutChanged)
	Q_PROPERTY(int	chosenColumn	READ proxyParentColumn	WRITE setProxyParentColumn	NOTIFY proxyParentColumnChanged)
	Q_PROPERTY(bool visible			READ visible			WRITE setVisible			NOTIFY visibleChanged)

public:
	LabelModel(DataSetPackage * package);

	bool labelNeedsFilter(size_t col);
	std::string columnName(size_t col) { return _package->getColumnName(col); }

	std::vector<bool>			filterAllows(size_t col);
	std::vector<std::string>	labels(size_t col);

	bool setData(const QModelIndex & index, const QVariant & value, int role)			override;

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

#endif // LABELMODEL_H
