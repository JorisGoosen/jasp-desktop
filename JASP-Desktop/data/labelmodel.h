#ifndef LABELMODEL_H
#define LABELMODEL_H


#include "datasettableproxy.h"

class LabelModel : public DataSetTableProxy
{
	Q_OBJECT

public:
	LabelModel(DataSetPackage * package);

	bool labelNeedsFilter(size_t col);
	std::string columnName(size_t col) { return _package->getColumnName(col); }

	std::vector<bool>			filterAllows(size_t col);
	std::vector<std::string>	labels(size_t col);
};

#endif // LABELMODEL_H
