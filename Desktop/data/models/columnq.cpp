#include "timers.h"
#include "labelq.h"
#include "columnq.h"
#include "datasetq.h"
#include "columnutils.h"
#include "utilities/qutils.h"
#include "../datasetpackageenums.h"

ColumnQ::ColumnQ(DataSetQ *data, int id)
	: Column(data, id)
{
	connect(this, &ColumnQ::manualEditMade,		data, &DataSetQ::manualEditMade				);
	connect(this, &ColumnQ::modelReset,			data, &DataSetQ::refresh					);
	connect(this, &ColumnQ::columnChanged,		data, &DataSetQ::handleColumnChanged		);
	connect(this, &ColumnQ::labelsReordered,	data, &DataSetQ::handleLabelsReordered		);
	connect(this, &ColumnQ::columnTypeChanged,	data, &DataSetQ::handleColumnTypeChanged	);
	connect(this, &ColumnQ::runFilter,			data, &DataSetQ::runFilter					);
	connect(this, &ColumnQ::labelFilterChanged,	data, &DataSetQ::labelFilterChanged			);
	connect(this, &ColumnQ::showWarning,		data, &DataSetQ::showWarning				);

	_emitLabelFilterChanged = [&](){ emit this->labelFilterChanged();		};
	_emitTypeChanged		= [&](){ emit this->columnTypeChanged(this);	};
	_beginResetModel		= [&](){ emit this->beginResetModel();			};
	_endResetModel			= [&](){ emit this->endResetModel();			};
}

QHash<int, QByteArray> ColumnQ::roleNames() const
{
	static bool						set = false;
	static QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();

	if(!set)
	{
		for(const auto & enumString : dataPkgRolesToStringMap())
			roles[int(enumString.first)] = QString::fromStdString(enumString.second).toUtf8();

		set = true;
	}

	return roles;
}

int ColumnQ::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : Column::rowCount();
}

int ColumnQ::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : 3;
}

QVariant ColumnQ::headerData(int section, Qt::Orientation orientation, int role) const
{

	if (section < 0 || section >= (orientation == Qt::Horizontal ? columnCount() : rowCount()))
		return QVariant();
	
	JASPTIMER_SCOPE(ColumnQ::headerData);
	
	if(orientation == Qt::Vertical)
		switch(role)
		{
		default:
			return QVariant();
	
		case int(dataPkgRoles::maxRowHeaderString):
			return QString::number(rowCount()) + "XXX";
	
		case Qt::DisplayRole:
			return QVariant(section + 1);
		}
	else
	{
		switch(section)
		{
		case 0:		return tr("Filter");
		case 1:		return tr("Value");
		case 2:		return tr("Label");
		}
	}
	
	return QVariant();
}

QVariant ColumnQ::data(const QModelIndex &index, int role) const
{
	JASPTIMER_SCOPE(ColumnQ::data);
	
	if(index.row() >= rowCount() || index.row() < 0 || index.column() >= columnCount() || index.column() < 0)
		return QVariant();
	
	if(role == Qt::DisplayRole) //You can specifically ask for the role you want, but the default one will show something according with headerData
		role = [](int c){ return int(c == 0 ? dataPkgRoles::filter : c == 1 ? dataPkgRoles::value : dataPkgRoles::label); }(index.column());
	
	switch(role)
	{
	case int(dataPkgRoles::nonFilteredNumericValuesCount):	return nonFilteredNumericsCount();
	case int(dataPkgRoles::nonFilteredLevels):				return tq(nonFilteredLevels());
	case int(dataPkgRoles::valuesDblList):					return getColumnValuesAsDoubleList();
	case int(dataPkgRoles::description):					return index.row() >= labels().size() ? "" : tq(labels()[index.row()]->description());
	case int(dataPkgRoles::filter):							return index.row() >= labels().size() || labels()[index.row()]->filterAllows();
	case int(dataPkgRoles::value):							return tq(labelsTempValue(index.row()));
	case int(dataPkgRoles::lines):							return dataQ()->getDataSetViewLines(index.row() == 0, index.column() == 0, true, true);
	case int(dataPkgRoles::label):							[[fallthrough]];
	case Qt::DisplayRole:									return tq(labelsTempDisplay(index.row()));
	default:												return QVariant();
	}
}

bool ColumnQ::setData(const QModelIndex &index, const QVariant &value, int role)
{
	JASPTIMER_SCOPE(ColumnQ::setData);
	
	if(index.column() >= columnCount() || index.row() >= rowCount() || index.column() < 0 || index.row() < 0)
		return false;


	switch(role)
	{
	case int(dataPkgRoles::filter):
		if(value.typeId() != QMetaType::Bool) 
			return false;

		return setLabelAllowFilter(index.row(), value.toBool());

	case int(dataPkgRoles::description):
		return setLabelDescription(index.row(), value.toString());

	case int(dataPkgRoles::value):
		return setLabelValue(index.row(),  value.toString());

	case int(dataPkgRoles::label):
		return setLabelDisplay(index.row(), value.toString());
		
	default:
		return false;
	}
	  
}


bool ColumnQ::setLabelDescription(int labelRow, const QString & newDescription)
{
	JASPTIMER_SCOPE(ColumnQ::setLabelDescription);

	Label		*	label	= labelByRow(labelRow);

	if(labelDoubleDummy() == label)
		label = replaceDoublesTillLabelsRowWithLabels(labelRow);

	label->setDescription(newDescription.toStdString());

	emit dataChanged(index(labelRow, 0),	index(labelRow, columnCount()), {int(dataPkgRoles::description), Qt::DisplayRole});

	return true;
}

bool ColumnQ::setLabelDisplay(int labelRow, const QString &newLabel)
{
	JASPTIMER_SCOPE(ColumnQ::setLabelDisplay);

	Label			*	label		= labelByRow(labelRow);
	bool				aChange		= false,
						setManual	= false;

	if(labelDoubleDummy() == label)
	{
		label	= replaceDoublesTillLabelsRowWithLabels(labelRow);
		aChange = true;
	}

	if(label->setLabel(fq(newLabel)))
	{
		aChange = true;

		if(dataQ()->dataFileCanHaveLabels())
			setManual = true;
	}

	if(aChange)
	{
		emit columnChanged(this);
		emit dataSetShouldRefresh();

		if(setManual)
			emit manualEditMade();

		emit dataChanged(index(labelRow, 0),	index(labelRow, columnCount()), {int(dataPkgRoles::label), Qt::DisplayRole});
	}

	return aChange;
}

bool ColumnQ::setLabelValue(int labelRow, const QString &newLabelValue)
{
	JASPTIMER_SCOPE(ColumnQ::setLabelValue);

	Label			*	label		= labelByRow(labelRow);
	bool				aChange		= false,
						aNumber		= false;

	Json::Value originalValue = newLabelValue.toStdString();

	int		anInteger;
	double	aDouble;

	if(	(aNumber =	ColumnUtils::getDoubleValue(newLabelValue.toStdString(), aDouble))	)	originalValue = aDouble;
	if(				ColumnUtils::getIntValue(	newLabelValue.toStdString(), anInteger)	)	originalValue = anInteger;


	if(labelDoubleDummy() == label)
	{
		int		replaceTill	= -1;
		double	oldDouble	= labelsTempValueDouble(labelRow);

		if(aNumber)
		{
			int newHasRow	= labelsDoubleValueIsTempLabelRow(aDouble);

			if(!Utils::isEqual(aDouble, oldDouble))
			{
				assert(newHasRow != labelRow); //Because it shouldnt be the same after all
				replaceTill = std::max(labelRow, newHasRow);
			}

			if(replaceTill < 0 && replaceDoubleLabelFromRowWithDouble(labelRow, aDouble))
			{
				emit columnChanged(this);
				emit manualEditMade();
				emit dataChanged(index(labelRow, 0),	index(labelRow, columnCount()), {int(dataPkgRoles::value), Qt::DisplayRole});
				return true;
			}
		}

		//if no a number then we will have to replace everything anyway because we wont be able to sort otherwise
		if(replaceTill == -1 && autoSortByValue())
				replaceTill = labelsTempCount();

		label	= replaceDoublesTillLabelsRowWithLabels(replaceTill > -1 ? replaceTill : labelRow, oldDouble);
		aChange = true;
	}

	{
		// Here we will overwrite the original value with the new origval.
		// but if the label is the same as the original value we want to make the users life easier and replace it as well.
		// this makes sense if the user is changing a string or number. But if the user is recoding, so turning values from str => dbl
		// then we dont want to do this, because then the label should be different afterwards.

		//summarized:
		// if orgval == label then:
		// if (oldorigval == dbl && newOrigVal == dbl) || (olorigval != dbl && newOrigVal != dbl)  then replace both
		// if neworigval == dbl and oldorigval != dbl then replace only value

		// But only if we are allowed to change both because of https://github.com/jasp-stats/INTERNAL-jasp/issues/2680 (allow editing of only value/label and disable the other one for computed columns
		// which means that if this column is a computed column of scale type we are only allowed to change the label and only the value for the other types.
		// so in this case this means that if it is a computed column, and of type !scale we do *not* also update the label when updating the value. Because otherwise it would override the data from the computed column...

		bool dontSetLabel = label->originalValueAsString(false) != label->labelDisplay() || (originalValue.isDouble() && !label->originalValue().isDouble());

		if(!dontSetLabel && isComputed() && type() != columnType::scale)
			dontSetLabel = true;

		if(dontSetLabel)	aChange = label->setOriginalValue(originalValue)	||	aChange;
		else				aChange = label->setOrigValLabel(originalValue)		||	aChange;
	}

	labelsHandleAutoSort();

	if(aChange)
	{
		emit columnChanged(this);
		emit manualEditMade();
	}

	return aChange;
}

bool ColumnQ::setLabelAllowFilter(int labelRow, bool newAllowValue)
{
	JASPTIMER_SCOPE(ColumnQ::setLabelAllowFilter);

	Label			*	label = labelByRow(labelRow);
	bool	atLeastOneRemains = newAllowValue;

	if(labelDoubleDummy() == label)
		replaceDoublesTillLabelsRowWithLabels(labelRow);

	if(!atLeastOneRemains) //Do not let the user uncheck every single one because that is useless, the user wants to uncheck row so lets see if there is another one left after that.
		for(size_t i=0; i< labels().size(); i++)
		{
			if(i != labelRow && labels()[i]->filterAllows())
			{
				atLeastOneRemains = true;
				break;
			}
			else if(i == labelRow && labels()[i]->filterAllows() == newAllowValue) //Did not change!
				return true;
		}

	atLeastOneRemains = atLeastOneRemains || labelsTempCount() > labels().size();

	if(atLeastOneRemains)
	{
		bool before = hasLabelFilter();
		labelByRow(labelRow)->setFilterAllows(newAllowValue);

		if(before != hasLabelFilter())
			emit dataSetShouldRefresh();

		emit labelFilterChanged();
		emit dataChanged(index(labelRow, 0), index(labelRow, columnCount()), { int(dataPkgRoles::filter), Qt::DisplayRole });

		return true;
	}
	else
		return false;
}

DataSetQ * ColumnQ::dataQ() const
{
	return static_cast<DataSetQ*>(Column::data());
}

QList<QVariant> ColumnQ::getColumnValuesAsDoubleList()	const
{
	QList<QVariant> list;

	for (double value : dbls())
		list.append(value);

	return list;
}

Label * ColumnQ::connectNewLabel(LabelQ * newLabel)
{
	connect(newLabel, &LabelQ::manualEditMade,		this, &ColumnQ::manualEditMade);
	connect(newLabel, &LabelQ::labelFilterChanged,	this, &ColumnQ::labelFilterChanged);
	
	return newLabel;
}

Label * ColumnQ::_createLabel()
{
	return connectNewLabel(new LabelQ(this));
}

Label * ColumnQ::_createLabel(int value)
{
	return connectNewLabel(new LabelQ(this, value));
}

Label * ColumnQ::_createLabel(const std::string &label, int value, bool filterAllows, const std::string &description, const Json::Value &originalValue, int order, int id)
{
	return connectNewLabel(new LabelQ(this, label, value, filterAllows, description, originalValue, order, id));
}

std::string	ColumnQ::generateLabelFilter() const
{
	JASPTIMER_SCOPE(ColumnQ::generateLabelFilter);

	boolvec				filterAllows	= getFilterAllows();
	stringvec			labels			= labelsTemp();
	int					pos				= std::count_if(filterAllows.begin(), filterAllows.end(), [](bool f){ return f; }),
						cnt				= 0;
	bool				bePositive		= pos <= filterAllows.size() - pos;
	std::stringstream	out;

	for(size_t row=0; row<filterAllows.size(); row++)
		if(filterAllows[row] == bePositive)
			out << (cnt++ > 0 ? (bePositive ? " | " : " & ") : "")
				<< name()
				<< ".nominal"	//Also make sure we use .nominal because otherwise we might be comparing to the value instead...
				<< (bePositive ? " == \"" : " != \"")
				<< labels[row] << "\"";

	return "(" + out.str() + ")";
}

boolvec ColumnQ::getFilterAllows() const
{
	boolvec list;
	list.reserve(labelsTempCount());

	for (const Label * label : labels())
		list.push_back(label->filterAllows());

	while(list.size() < labelsTempCount())
		list.push_back(true);

	return list;
}

void ColumnQ::resetFilterAllows()
{
	resetFilter();
	nonFilteredCountersReset();

	emit columnChanged(this);
}


