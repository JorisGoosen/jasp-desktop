
#include "levelstablemodel.h"


#include <algorithm>

#include <QColor>

#include "utilities/qutils.h"

LevelsTableModel::LevelsTableModel(DataSetPackage * package) : DataSetTableProxy(package, parIdxType::label)
{
	//_column = NULL;
	//connect(this, &LevelsTableModel::refreshConnectedModels, this, &LevelsTableModel::refreshConnectedModelsToName);
	connect(_package, &DataSetPackage::filteredOutChanged, this, &LevelsTableModel::filteredOutChangedHandler);
}

void LevelsTableModel::_moveRows(QModelIndexList &selection, bool up)
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

void LevelsTableModel::moveUp(QModelIndexList &selection)
{
	_moveRows(selection, true);
}

void LevelsTableModel::moveDown(QModelIndexList &selection)
{
	_moveRows(selection, false);
}

void LevelsTableModel::reverse()
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

QModelIndexList LevelsTableModel::convertQVariantList_to_QModelIndexList(QVariantList selection)
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

bool LevelsTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	int roleToSet = index.column() == 0 ? int(DataSetPackage::specialRoles::filter) : index.column() == 1 ? int(DataSetPackage::specialRoles::value) : Qt::DisplayRole;
	return _package->setData(mapToSource(index), value, roleToSet);
}

void LevelsTableModel::filteredOutChangedHandler(int c)
{
	if(c == proxyParentColumn()) emit filteredOutChanged();
}

int LevelsTableModel::filteredOut() const
{
	return _package->filteredOut(proxyParentColumn());
}

void LevelsTableModel::resetFilterAllows()
{
	_package->resetFilterAllows(proxyParentColumn());
}

void LevelsTableModel::setVisible(bool visible)
{
	if (_visible == visible)
		return;

	_visible = visible;
	emit visibleChanged(_visible);
}

/*
void LevelsTableModel::setColumn(Column *column)
{
	beginResetModel();
	_column = column;
	_colName = column != NULL ? column->name() : "";
	endResetModel();
	emit resizeLabelColumn();

	if(column == NULL)	setChosenColumn(-1);
	else				setChosenColumn(_dataSet->getColumnIndex(_colName));
}

void LevelsTableModel::refreshColumn(Column * column)
{
	if(_column != NULL && _column == column)
		refresh();
}

void LevelsTableModel::refresh()
{
	beginResetModel();
	endResetModel();
	emit resizeLabelColumn();
	emit filteredOutChanged();
}

void LevelsTableModel::clearColumn()
{
	setColumn(NULL);
}

void LevelsTableModel::setDataSet(DataSet * thisDataSet)
{
	_dataSet = thisDataSet;

	int newIndex = _dataSet == NULL ? -1 : _dataSet->getColumnIndex(_colName);

	if(newIndex == -1)	clearColumn(); //also resets model
	else				setColumn(&(_dataSet->column(newIndex)));
}

int LevelsTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	if (_column == NULL)
		return 0;

	return _column->labels().size();
}

int LevelsTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 3;
}

QVariant LevelsTableModel::data(const QModelIndex &index, int role) const
{
	if (_dataSet == nullptr || _dataSet->synchingData())
		return QVariant();

	if (role == Qt::BackgroundColorRole && index.column() == 0)
		return QColor(0xf6,0xf6,0xf6);

	Labels &labels = _column->labels();
	int row = index.row();

	if(row < 0 || row >= rowCount())
		return QVariant();


	if(role == (int)Roles::ValueRole)	return tq(labels.getValueFromRow(row));
	if(role == (int)Roles::LabelRole)	return tq(labels.getLabelFromRow(row));
	if(role == (int)Roles::FilterRole)	return QVariant(labels[row].filterAllows());

	if(role == Qt::DisplayRole)
		switch(index.column())
		{
		case 0:	return tq(labels.getValueFromRow(row));
		case 1:	return tq(labels.getLabelFromRow(row));
		case 2:	return QVariant(labels[row].filterAllows());
		}

	return QVariant();
}

QVariant LevelsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == (int)Roles::ValueRole)	return "Value";
	if(role == (int)Roles::LabelRole)	return "Label";
	if(role == (int)Roles::FilterRole)	return "Filter";

	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation != Qt::Horizontal)
		return QVariant();

	if (section == 0)	return "Value";
	else				return "Label";
}
*/

/*
Qt::ItemFlags LevelsTableModel::flags(const QModelIndex &index) const
{
	if (index.column() == 1) {
        return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    } else {
        return QAbstractTableModel::flags(index);
	}
}

bool LevelsTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (_column == NULL)
        return false;

    if (role == Qt::EditRole)
    {
        const std::string &new_label = value.toString().toStdString();
        if (new_label != "") {
            Labels &labels = _column->labels();
			if (labels.setLabelFromRow(index.row(), new_label))
			{
				emit dataChanged(index, index);

				emit refreshConnectedModels(_column);

				emit labelFilterChanged();
			}

		}
    }

    return true;
}


QHash<int, QByteArray> LevelsTableModel::roleNames() const
{
	static const QHash<int, QByteArray> roles = QHash<int, QByteArray> { {(int)Roles::ValueRole, "value"}, {(int)Roles::LabelRole, "label"}, {(int)Roles::FilterRole, "filter"} };
	return roles;
}
*/

/*int LevelsTableModel::currentColumnIndex()
{
	if(_column == NULL)
		return -1;

	try			{ return _dataSet->columns().findIndexByName(_column->name()); }
	catch(...)	{ return -1; }
}*/


/*
void LevelsTableModel::setChosenColumn(int chosenColumn)
{
	if (_chosenColumn == chosenColumn)
		return;

	_chosenColumn = chosenColumn;
	emit chosenColumnChanged(_chosenColumn);
}*/
