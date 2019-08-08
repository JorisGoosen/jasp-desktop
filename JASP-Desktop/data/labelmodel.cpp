#include "labelmodel.h"

LabelModel::LabelModel(DataSetPackage * package) : DataSetTableProxy(package, parIdxType::label)
{
	connect(_package, &DataSetPackage::filteredOutChanged, this, &LabelModel::filteredOutChangedHandler);
}

bool LabelModel::labelNeedsFilter(size_t col)
{
	QVariant result = headerData(col, Qt::Orientation::Horizontal, int(DataSetPackage::specialRoles::labelsHasFilter));

	if(result.type() == QMetaType::Bool)	return result.toBool();
	return false;
}

std::vector<bool> LabelModel::filterAllows(size_t col)
{
	std::vector<bool> allows(rowCount());

	for(size_t row=0; row<rowCount(); row++)
		allows[row] = data(index(row, col), int(DataSetPackage::specialRoles::filter)).toBool();

	return allows;
}

std::vector<std::string> LabelModel::labels(size_t col)
{
	std::vector<std::string> labels(rowCount());

	for(size_t row=0; row<rowCount(); row++)
		labels[row] = data(index(row, col), Qt::DisplayRole).toString().toStdString();

	return labels;
}

void LabelModel::_moveRows(QModelIndexList &selection, bool up)
{
	/*if (_column == NULL)
		return;


	Labels &labels = _column->labels();
	std::vector<Label> new_labels(labels.begin(), labels.end());

	for (QModelIndex &index : selection)
	{
		//if (index.column() == 0) {
			iter_swap(new_labels.begin() + index.row(), new_labels.begin() + (index.row() + (up ? - 1: 1)));
		//}
	}*/

	beginResetModel();
//	labels.set(new_labels);
	endResetModel();

}

void LabelModel::moveUp(QModelIndexList &selection)
{
	_moveRows(selection, true);
}

void LabelModel::moveDown(QModelIndexList &selection)
{
	_moveRows(selection, false);
}

void LabelModel::reverse()
{
  /*  if (_column == NULL)
		return;*/

	beginResetModel();
/*
	Labels &labels = _column->labels();
	std::vector<Label> new_labels(labels.begin(), labels.end());

	std::reverse(new_labels.begin(), new_labels.end());

	labels.set(new_labels);*/

	/*QModelIndex topLeft = createIndex(0,0);
	QModelIndex bottonRight = createIndex(labels.size() - 1, 1);
	//emit a signal to make the view reread identified data
	emit dataChanged(topLeft, bottonRight);*/
	endResetModel();
}

QModelIndexList LabelModel::convertQVariantList_to_QModelIndexList(QVariantList selection)
{
	QModelIndexList List;
	bool Converted;

	for(QVariant variant : selection)
	{
		int Row = variant.toInt(&Converted);
		if(Converted)
			List << index(Row, 0);
	}

	return List;

}

bool LabelModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	int roleToSet = index.column() == 0 ? int(DataSetPackage::specialRoles::filter) : index.column() == 1 ? int(DataSetPackage::specialRoles::value) : Qt::DisplayRole;
	return _package->setData(mapToSource(index), value, roleToSet);
}

void LabelModel::filteredOutChangedHandler(int c)
{
	if(c == proxyParentColumn()) emit filteredOutChanged();
}

int LabelModel::filteredOut() const
{
	return _package->filteredOut(proxyParentColumn());
}

void LabelModel::resetFilterAllows()
{
	_package->resetFilterAllows(proxyParentColumn());
}

void LabelModel::setVisible(bool visible)
{
	if (_visible == visible)
		return;

	_visible = visible;
	emit visibleChanged(_visible);
}

