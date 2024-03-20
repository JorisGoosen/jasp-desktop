#include "columnmodel.h"
#include "jasptheme.h"
#include "utilities/qutils.h"
#include "datasettablemodel.h"
#include "computedcolumnmodel.h"

QMap<computedColumnType, QString> ColumnModel::columnTypeFriendlyName =
{
	std::make_pair(computedColumnType::notComputed,			QObject::tr("Not computed")),
	std::make_pair(computedColumnType::rCode,				QObject::tr("Computed with R code")),
	std::make_pair(computedColumnType::constructorCode,		QObject::tr("Computed with drag-and-drop")),
	std::make_pair(computedColumnType::analysis,			QObject::tr("Column generated by analysis")),
	std::make_pair(computedColumnType::analysisNotComputed, QObject::tr("Column added by analysis"))
};

QVariantList ColumnModel::computedTypeValues() const
{
	static QVariantList computedChoices, analysisChoice, analysisNotComputedChoice;

	if (computedChoices.isEmpty())
	{
		QMap<QString, QVariant> notComputedChoice =		{ std::make_pair("value", computedColumnTypeToQString(computedColumnType::notComputed)),		std::make_pair("label", columnTypeFriendlyName[computedColumnType::notComputed])};
		QMap<QString, QVariant> rCodeChoice =			{ std::make_pair("value", computedColumnTypeToQString(computedColumnType::rCode)),				std::make_pair("label", columnTypeFriendlyName[computedColumnType::rCode])};
		QMap<QString, QVariant> constructorCodeChoice = { std::make_pair("value", computedColumnTypeToQString(computedColumnType::constructorCode)),	std::make_pair("label", columnTypeFriendlyName[computedColumnType::constructorCode])};
		computedChoices.push_back(notComputedChoice);
		computedChoices.push_back(rCodeChoice);
		computedChoices.push_back(constructorCodeChoice);
	}

	if (analysisChoice.isEmpty())
	{
		QMap<QString, QVariant> uniqueChoice =			{ std::make_pair("value", computedColumnTypeToQString(computedColumnType::analysis)),			std::make_pair("label", columnTypeFriendlyName[computedColumnType::analysis])};
		analysisChoice.push_back(uniqueChoice);
	}

	if (analysisNotComputedChoice.isEmpty())
	{
		QMap<QString, QVariant> uniqueChoice =			{ std::make_pair("value", computedColumnTypeToQString(computedColumnType::analysisNotComputed)), std::make_pair("label", columnTypeFriendlyName[computedColumnType::analysisNotComputed])};
		analysisNotComputedChoice.push_back(uniqueChoice);
		analysisNotComputedChoice.append(computedChoices);
	}

	computedColumnType type = column() ? column()->codeType() : computedColumnType::notComputed;

	switch(type)
	{
	case computedColumnType::notComputed:
	case computedColumnType::rCode:
	case computedColumnType::constructorCode:
		return computedChoices;

	case computedColumnType::analysis:
		return analysisChoice;

	case computedColumnType::analysisNotComputed:
		return analysisNotComputedChoice;
	}
	
	return {};
}

QVariantList ColumnModel::columnTypeValues() const
{
	static QVariantList columnTypes;

	if (columnTypes.isEmpty())
	{
		QMap<QString, QVariant> scaleChoice =			{ std::make_pair("value", columnTypeToQString(columnType::scale)),				std::make_pair("label", QObject::tr("Scale")),		std::make_pair("columnTypeIcon", JaspTheme::currentIconPath() + "variable-scale.png")};
		QMap<QString, QVariant> ordinalChoice =			{ std::make_pair("value", columnTypeToQString(columnType::ordinal)),			std::make_pair("label", QObject::tr("Ordinal")),	std::make_pair("columnTypeIcon", JaspTheme::currentIconPath() + "variable-ordinal.png")};
		QMap<QString, QVariant> nominalChoice =			{ std::make_pair("value", columnTypeToQString(columnType::nominal)),			std::make_pair("label", QObject::tr("Nominal")),	std::make_pair("columnTypeIcon", JaspTheme::currentIconPath() + "variable-nominal.png")};
		columnTypes.push_back(scaleChoice);
		columnTypes.push_back(ordinalChoice);
		columnTypes.push_back(nominalChoice);
	}

	return columnTypes;
}

ColumnModel::ColumnModel(DataSetTableModel* dataSetTableModel)
	: DataSetTableProxy(DataSetPackage::pkg()->labelsSubModel()),
	  _dataSetTableModel{dataSetTableModel}
{
	connect(DataSetPackage::pkg(),	&DataSetPackage::filteredOutChanged,			this, &ColumnModel::filteredOutChangedHandler	);
	connect(this,					&DataSetTableProxy::nodeChanged,				this, &ColumnModel::filteredOutChanged			);
	connect(this,					&DataSetTableProxy::nodeChanged,				this, &ColumnModel::refresh						);
	connect(this,					&DataSetTableProxy::nodeChanged,				this, &ColumnModel::chosenColumnChanged			);
	connect(this,					&ColumnModel::chosenColumnChanged,				this, &ColumnModel::onChosenColumnChanged		);

	connect(DataSetPackage::pkg(),	&DataSetPackage::modelReset,					this, &ColumnModel::refresh						);
	connect(DataSetPackage::pkg(),	&DataSetPackage::allFiltersReset,				this, &ColumnModel::allFiltersReset				);
	connect(DataSetPackage::pkg(),	&DataSetPackage::labelFilterChanged,			this, &ColumnModel::labelFilterChanged			);
	connect(DataSetPackage::pkg(),	&DataSetPackage::columnDataTypeChanged,			this, &ColumnModel::columnDataTypeChanged		);
	connect(DataSetPackage::pkg(),	&DataSetPackage::labelsReordered,				this, &ColumnModel::refresh						);
	connect(DataSetPackage::pkg(),	&DataSetPackage::columnsBeingRemoved,			this, &ColumnModel::checkRemovedColumns			);
	connect(DataSetPackage::pkg(),	&DataSetPackage::columnsInserted,				this, &ColumnModel::checkInsertedColumns		);
	connect(DataSetPackage::pkg(),	&DataSetPackage::datasetChanged,				this, &ColumnModel::checkCurrentColumn			);
	connect(DataSetPackage::pkg(),	&DataSetPackage::workspaceEmptyValuesChanged,	this, &ColumnModel::emptyValuesChanged			);

	_undoStack = DataSetPackage::pkg()->undoStack();
}

QString ColumnModel::columnNameQ()
{
	if (_virtual) return _dummyColumn.name;

	return QString::fromStdString(column() ? column()->name() : "");
}


void ColumnModel::setColumnNameQ(QString newColumnName)
{
	if (newColumnName == columnNameQ()) return;

	if (_virtual)
	{
		_undoStack->startMacro();

		for (int colNr = _dataSetTableModel->columnCount(); colNr < _currentColIndex; colNr++)
			_undoStack->pushCommand(new InsertColumnCommand(_dataSetTableModel, colNr));

		QMap<QString, QVariant> props;
		props["name"] = newColumnName;
		props["type"] = int(_dummyColumn.type);
		_undoStack->endMacro(new InsertColumnCommand(_dataSetTableModel, _currentColIndex, props));
	}
	else if(column())
		_undoStack->pushCommand(new SetColumnPropertyCommand(this, newColumnName, SetColumnPropertyCommand::ColumnProperty::Name));
}

QString ColumnModel::columnTitle() const
{
	if (_virtual) return _dummyColumn.title;

	return QString::fromStdString(column() ? column()->title() : "");
}

void ColumnModel::setColumnTitle(const QString & newColumnTitle)
{
	if (_virtual)
		_dummyColumn.title = newColumnTitle;

	if(column() && column()->title() != fq(newColumnTitle))
		_undoStack->pushCommand(new SetColumnPropertyCommand(this, newColumnTitle, SetColumnPropertyCommand::ColumnProperty::Title));
}

QString ColumnModel::columnDescription() const
{
	if (_virtual) return _dummyColumn.description;

	return QString::fromStdString(column() ? column()->description() : "");
}

bool ColumnModel::useCustomEmptyValues() const
{
	if (_virtual || !column()) return false;

	return column()->hasCustomEmptyValues();
}

void ColumnModel::setUseCustomEmptyValues(bool useCustom)
{
	if (_virtual || !column()) return;

	_undoStack->pushCommand(new SetUseCustomEmptyValuesCommand(this, useCustom));
}

QStringList ColumnModel::emptyValues() const
{
	return (_virtual || !column()) ? QStringList() : tql(column()->emptyValues()->emptyStringsColumnModel());
}

void ColumnModel::setCustomEmptyValues(const QStringList& customEmptyValues)
{
	if (_virtual || !column()) return;

	_undoStack->pushCommand(new SetCustomEmptyValuesCommand(this, customEmptyValues));
}

void ColumnModel::addEmptyValue(const QString & value)
{
	QStringList values = emptyValues();
	values.push_back(value);
	setCustomEmptyValues(values);
}

void ColumnModel::removeEmptyValue(const QString & value)
{
	QStringList values = emptyValues();
	values.removeAll(value);
	setCustomEmptyValues(values);
}

void ColumnModel::resetEmptyValues()
{
	setCustomEmptyValues(tql(DataSetPackage::pkg()->workspaceEmptyValues()));
}


QVariantList ColumnModel::tabs() const
{
	QVariantList tabs;
	Column* col = column();
	
	if(_compactMode)
		tabs.push_back(QMap<QString, QVariant>({  std::make_pair("name", "basicInfo"), std::make_pair("title", tr("Column definition"))}));
	
	if(col)
	{
		if (col->isComputed() && (col->codeType() == computedColumnType::rCode || col->codeType() == computedColumnType::constructorCode))
			tabs.push_back(QMap<QString, QVariant>({  std::make_pair("name", "computed"), std::make_pair("title", tr("Computed column definition"))}));

		if (rowCount() > 0)
			tabs.push_back(QMap<QString, QVariant>({  std::make_pair("name", "label"), std::make_pair("title", tr("Label editor"))}));
	}

	QMap<QString, QVariant> misingValues =	{  std::make_pair("name", "missingValues"), std::make_pair("title", tr("Missing Values"))};
	tabs.push_back(misingValues);

	return tabs;
}


QString ColumnModel::currentColumnType() const
{
	if (_virtual) return columnTypeToQString(_dummyColumn.type);

	columnType type = column() ? column()->type() : columnType::scale;

	return columnTypeToQString(type);
}

QString ColumnModel::computedType() const
{
	if (_virtual) return computedColumnTypeToQString(_dummyColumn.computedType);

	return column() ? computedColumnTypeToQString(column()->codeType()) : computedColumnTypeToQString(computedColumnType::notComputed);
}

bool ColumnModel::computedTypeEditable() const
{
	if(_virtual)
		return true;

	if (!column())
		return false;

	switch (column()->codeType())
	{
	case computedColumnType::notComputed:
	case computedColumnType::analysisNotComputed:
	case computedColumnType::constructorCode:
	case computedColumnType::rCode:
		return true;

	default:
		return false;
	}
}

void ColumnModel::setColumnDescription(const QString & newColumnDescription)
{
	if (_virtual)
		_dummyColumn.description = newColumnDescription;

	if(column() && column()->description() != fq(newColumnDescription))
		_undoStack->pushCommand(new SetColumnPropertyCommand(this, newColumnDescription, SetColumnPropertyCommand::ColumnProperty::Description));
}

void ColumnModel::setComputedType(QString type)
{
	if (type == computedType())
		return;

	computedColumnType cType = computedColumnTypeFromString(type.toStdString());

	if (_virtual)
		_dummyColumn.computedType = cType;
	else if(column())
		_undoStack->pushCommand(new SetColumnPropertyCommand(this, int(cType), SetColumnPropertyCommand::ColumnProperty::ComputedColumn));

	emit ComputedColumnModel::singleton()->refreshProperties();
	emit tabsChanged();
}

void ColumnModel::setColumnType(QString type)
{
	if (type.isEmpty() || type == currentColumnType()) return;

	columnType cType = columnTypeFromString(type.toStdString());

	if (_virtual)
		_dummyColumn.type = cType;
	else if(column())
		_undoStack->pushCommand(new SetColumnTypeCommand(this, chosenColumn(), int(cType)));

	emit tabsChanged();
}

std::vector<qsizetype> ColumnModel::getSortedSelection() const
{
	if (_virtual) return {};

	std::map<QString, qsizetype> mapValueToRow;

	for(qsizetype r=0; r<qsizetype(rowCount()); r++)
		mapValueToRow[data(index(r, 0), int(DataSetPackage::specialRoles::value)).toString()] = r;

	std::vector<qsizetype> out;

	for(const QString & v : _selected)
		out.push_back(mapValueToRow[v]);

	std::sort(out.begin(), out.end());

	return out;
}

void ColumnModel::setValueMaxWidth()
{
	qsizetype maxWidthChars = std::max((tr("Value").size()), !column() ? 0 : column()->getMaximumWidthInCharacters(false, true));
	
	double prevMaxWidth = _valueMaxWidth;
	_valueMaxWidth = JaspTheme::fontMetrics().size(Qt::TextSingleLine, QString(maxWidthChars, 'X')).width();

	if(_valueMaxWidth != prevMaxWidth)
		emit valueMaxWidthChanged();
}

void ColumnModel::setLabelMaxWidth()
{
	qsizetype maxWidthChars = std::max((tr("Label").size()), !column() ? 0 : column()->getMaximumWidthInCharacters(false, false));
	
	double prevMaxWidth = _labelMaxWidth;
	_labelMaxWidth = JaspTheme::fontMetrics().size(Qt::TextSingleLine, QString(maxWidthChars, 'X')).width();

	if(_labelMaxWidth != prevMaxWidth)
		emit labelMaxWidthChanged();
}

void ColumnModel::moveSelectionUp()
{
	std::vector<qsizetype> indexes = getSortedSelection();
	if (indexes.size() < 1)
		return;

	_lastSelected = -1;
	_undoStack->pushCommand(new MoveLabelCommand(this, indexes, true));
}

void ColumnModel::moveSelectionDown()
{
	std::vector<qsizetype> indexes = getSortedSelection();
	if (indexes.size() < 1)
		return;

	_lastSelected = -1;
	_undoStack->pushCommand(new MoveLabelCommand(this, indexes, false));
}

void ColumnModel::reverse()
{
	_lastSelected = -1;
	_undoStack->pushCommand(new ReverseLabelCommand(this));
}

bool ColumnModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if(role == int(DataSetPackage::specialRoles::selected))
		return false;

	bool result = DataSetTableProxy::setData(index, value, role);

	if (!_editing && (role == Qt::EditRole || role == int(DataSetPackage::specialRoles::filter)))
		setSelected(index.row(), 0);

	return result;
}

QVariant ColumnModel::data(	const QModelIndex & index, int role) const
{
	if(role == int(DataSetPackage::specialRoles::selected))
	{
		bool s = _selected.count(data(index, int(DataSetPackage::specialRoles::value)).toString()) > 0;
		return s;
	}

	return DataSetTableProxy::data(index, role > 0 ? role : int(DataSetPackage::specialRoles::label));
}

void ColumnModel::filteredOutChangedHandler(int c)
{
	JASPTIMER_SCOPE(ColumnModel::filteredOutChangedHandler);

	if(c == chosenColumn())
		emit filteredOutChanged();
}

int ColumnModel::filteredOut() const
{
	return DataSetPackage::pkg()->filteredOut(chosenColumn());
}

void ColumnModel::resetFilterAllows()
{
	DataSetPackage::pkg()->resetFilterAllows(chosenColumn());
}

void ColumnModel::setVisible(bool visible)
{
	//visible = visible && rowCount() > 0; //cannot show labels when there are no labels

	if (_visible == visible)
		return;

	_visible = visible;
	emit visibleChanged(_visible);
}

Column * ColumnModel::column() const 
{
	if (_virtual) return nullptr;

	return static_cast<Column *>(node());
}

int ColumnModel::chosenColumn() const
{
	Column * c = column();
	
	if(!c)
		return -1;
	
	return c->data()->columnIndex(c);
}

void ColumnModel::setChosenColumn(int chosenColumn)
{
	// Always set the chosen column even if it is the same one: the ColumnModel might be not reset correctly when the dataset is closed.

	//If the user deletes the name the column ought to be removed because we cannot have columns without a name!
	int deleteMe = column() && column()->name() == "" ? _currentColIndex : -1;

	emit beforeChangingColumn(chosenColumn);

	//This only works as long as we have a single dataSet but lets not go overboard with rewriting stuff atm
	DataSet * data = DataSetPackage::pkg()->dataSet();
	clearVirtual();

	_virtual = data ? (chosenColumn >= data->columnCount()) : true;
	emit isVirtualChanged();

	subNodeModel()->selectNode(!_virtual ? data->column(chosenColumn) : nullptr);

	_currentColIndex = chosenColumn;

	refresh();

	if(deleteMe >= 0)
		_dataSetTableModel->removeColumn(deleteMe);
}

void ColumnModel::setChosenColumn(const QString & chosenName)
{
	DataSet * data = DataSetPackage::pkg()->dataSet();
	Column* col = data->column(fq(chosenName));
	setChosenColumn(data->columnIndex(col));
}


void ColumnModel::columnDataTypeChanged(const QString & colName)
{
	int colIndex = DataSetPackage::pkg()->getColumnIndex(colName);

	if(colIndex == chosenColumn())
	{
		emit columnTypeChanged();
//		if(DataSetPackage::pkg()->dataSet()->column(colIndex)->type() == columnType::scale)
//			setChosenColumn(-1);
		emit tabsChanged();
		invalidate();
	}
}

void ColumnModel::setRowWidth(double len)
{
	if (abs(len - _rowWidth) > 0.001)
	{
		_rowWidth = len;

		emit headerDataChanged(Qt::Horizontal, 0, 0);
	}
}

///Override of headerData because it doesnt get QModelIndex and thus cannot know whether it is proxied by columnModel or something else...
QVariant ColumnModel::headerData(int section, Qt::Orientation orientation, int role)	const
{
	if (section < 0 || section >= (orientation == Qt::Horizontal ? columnCount() : rowCount()))
		return QVariant();

	switch(role)
	{
	case int(DataSetPackage::specialRoles::columnWidthFallback):	return _rowWidth;
	case int(DataSetPackage::specialRoles::maxRowHeaderString):		return "";
	case Qt::DisplayRole:											return QVariant(section);
	case Qt::TextAlignmentRole:										return QVariant(Qt::AlignCenter);
	}

	return QVariant();
}

int ColumnModel::rowCount(const QModelIndex & p) const
{
	if(p.isValid())
		return 0;
	
	return !column() ? 0 : column()->labelsTempCount(); //Im having some trouble with the proxymodel, so lets take a shortcut 
}

void ColumnModel::onChosenColumnChanged()
{
	_selected.clear();
	_lastSelected = -1;
	setValueMaxWidth();
	setLabelMaxWidth();
	
	ComputedColumnModel::singleton()->selectColumn(column());
}

void ColumnModel::refresh()
{	
	beginResetModel();
	endResetModel();

	emit columnNameChanged();
	emit nameEditableChanged();
	emit columnTitleChanged();
	emit columnDescriptionChanged();
	emit computedTypeValuesChanged();
	emit computedTypeChanged();
	emit computedTypeEditableChanged();
	emit columnTypeChanged();
	emit useCustomEmptyValuesChanged();
	emit emptyValuesChanged();

	emit tabsChanged();

	setValueMaxWidth();
	setLabelMaxWidth();
}

void ColumnModel::changeSelectedColumn(QPoint selectionStart)
{
	if (selectionStart.x() != chosenColumn() && visible())
		setChosenColumn(selectionStart.x());
}

void ColumnModel::checkInsertedColumns(const QModelIndex &, int first, int)
{
	if (_currentColIndex >= first)
	{
		_currentColIndex = -1; // Force the setting of new column.
		setChosenColumn(first);
	}
}

void ColumnModel::checkRemovedColumns(int columnIndex, int count)
{
	int currentCol = chosenColumn();
	if ((columnIndex <= currentCol) && (currentCol < columnIndex + count))
	{
		setVisible(false);
		setChosenColumn(-1);
	}
}

void ColumnModel::openComputedColumn(const QString & name)
{
	setChosenColumn(name);
	setVisible(true);
}

void ColumnModel::checkCurrentColumn(QStringList, QStringList missingColumns, QMap<QString, QString> changeNameColumns, bool, bool hasNewColumns)
{
	QString colName = columnNameQ();

	if (missingColumns.contains(colName))
	{
		setVisible(false);
		setChosenColumn(-1);
	}
	else
	{
		if (!_virtual && changeNameColumns.contains(colName))
			setColumnNameQ(changeNameColumns[colName]);
		if (hasNewColumns && _virtual && DataSetPackage::pkg()->dataColumnCount() >= _currentColIndex)
		{
			// The current column is not virtual anymore: reset it
			int colId = _currentColIndex;
			_currentColIndex = -1;
			setChosenColumn(colId);
		}
	}
}

void ColumnModel::removeAllSelected()
{
	QMap<QString, qsizetype> mapValueToRow;

	for(qsizetype r=0; r<qsizetype(rowCount()); r++)
		mapValueToRow[data(index(r, 0), int(DataSetPackage::specialRoles::value)).toString()] = r;

	QVector<QString> selectedValues;
	for (const QString& s : _selected)
		selectedValues.append(s);

	_selected.clear();
	_lastSelected = -1;
	for (const QString& selectedValue : selectedValues)
	{
		if (mapValueToRow.contains(selectedValue))
		{
			int selectedRow = int(mapValueToRow[selectedValue]);
			emit dataChanged(ColumnModel::index(selectedRow, 0), ColumnModel::index(selectedRow, 0), {int(DataSetPackage::specialRoles::selected)});
		}
	}
}

void ColumnModel::setSelected(int row, int modifier)
{
	if (modifier & Qt::ShiftModifier && _lastSelected >= 0)
	{
		int start = _lastSelected >= row ? row : _lastSelected;
		int end = start == _lastSelected ? row : _lastSelected;
		for (int i = start; i <= end; i++)
		{
			QString rowValue = data(index(i, 0), int(DataSetPackage::specialRoles::value)).toString();
			_selected.insert(rowValue);
			emit dataChanged(ColumnModel::index(i, 0), ColumnModel::index(i, 0), {int(DataSetPackage::specialRoles::selected)});
		}
	}
	else if (modifier & Qt::ControlModifier)
	{
		QString rowValue = data(index(row, 0), int(DataSetPackage::specialRoles::value)).toString();
		_selected.insert(rowValue);
		emit dataChanged(ColumnModel::index(row, 0), ColumnModel::index(row, 0), {int(DataSetPackage::specialRoles::selected)});
	}
	else
	{
		QString rowValue = data(index(row, 0), int(DataSetPackage::specialRoles::value)).toString();
		bool disableCurrent = _selected.count(rowValue) > 0;
		removeAllSelected();
		if (!disableCurrent)	_selected.insert(rowValue);
		else					_selected.erase(rowValue);
		emit dataChanged(ColumnModel::index(row, 0), ColumnModel::index(row, 0), {int(DataSetPackage::specialRoles::selected)});
	}
	_lastSelected = row;

}

void ColumnModel::unselectAll()
{
	_selected.clear();
	_lastSelected = -1;
	emit dataChanged(ColumnModel::index(0, 0), ColumnModel::index(rowCount(), 0), {int(DataSetPackage::specialRoles::selected)});
}

bool ColumnModel::setChecked(int rowIndex, bool checked)
{
	JASPTIMER_SCOPE(ColumnModel::setChecked);
	
	if(checked == data(index(rowIndex,0), int(DataSetPackage::specialRoles::filter)).toBool())
		return true; //Its already that value

	_editing = true;
	_undoStack->pushCommand(new FilterLabelCommand(this, rowIndex, checked));
	_editing = false;
	
	return data(index(rowIndex, 0), int(DataSetPackage::specialRoles::filter)).toBool() == checked;
}

void ColumnModel::setValue(int rowIndex, const QString &value)
{
	JASPTIMER_SCOPE(ColumnModel::setValue);
	
	if(value == data(index(rowIndex,0), int(DataSetPackage::specialRoles::value)).toString())
		return; //Its already that value
	
	_editing = true;
	_undoStack->pushCommand(new SetLabelOriginalValueCommand(this, rowIndex, value));
	_editing = false;
}

void ColumnModel::setLabel(int rowIndex, QString label)
{
	JASPTIMER_SCOPE(ColumnModel::setLabel);
	
	if(label == data(index(rowIndex,0), int(DataSetPackage::specialRoles::label)).toString())
		return; //Its already that value
	
	_editing = true;
	_undoStack->pushCommand(new SetLabelCommand(this, rowIndex, label));
	_editing = false;
}

bool ColumnModel::columnIsFiltered() const
{
	if(column())
		return column()->hasFilter();
	return false;
}

bool ColumnModel::nameEditable() const
{
	if(column())
		return !(column()->isComputed() && (column()->codeType() == computedColumnType::analysisNotComputed || column()->codeType() == computedColumnType::analysis));

	return true;
}

void ColumnModel::clearVirtual()
{
	_dummyColumn.description.clear();
	_dummyColumn.name.clear();
	_dummyColumn.title.clear();

	_dummyColumn.type			= columnType::scale;
	_dummyColumn.computedType	= computedColumnType::notComputed;
}

bool ColumnModel::compactMode() const
{
	return _compactMode;
}

void ColumnModel::setCompactMode(bool newCompactMode)
{
	if (_compactMode == newCompactMode)
		return;
	_compactMode = newCompactMode;
	emit compactModeChanged();
	
	emit tabsChanged();
}
