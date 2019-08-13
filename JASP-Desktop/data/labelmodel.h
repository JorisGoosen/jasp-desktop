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

	void moveUp(	std::vector<size_t> selection);
	void moveDown(	std::vector<size_t> selection);

	Q_INVOKABLE void reverse();

	Q_INVOKABLE void moveUpFromQML(QVariantList selection)		{ moveUp(	convertQVariantList_to_RowVec(selection)); }
	Q_INVOKABLE void moveDownFromQML(QVariantList selection)	{ moveDown(	convertQVariantList_to_RowVec(selection)); }

	std::vector<size_t> convertQVariantList_to_RowVec(QVariantList selection);

	Q_INVOKABLE void resetFilterAllows();

	bool	visible()			const {	return _visible; }
	int		filteredOut()		const;
	int		dataColumnCount()	const;

public slots:
	void filteredOutChangedHandler(int col);

	void setVisible(bool visible);

signals:
	void visibleChanged(bool visible);
	void filteredOutChanged();

private:
	bool	_visible		= false;

};

#endif // LABELMODEL_H
