#include "labelmodel.h"

LabelModel::LabelModel(DataSetPackage * package) : DataSetTableProxy(package, parIdxType::filter)
{

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

