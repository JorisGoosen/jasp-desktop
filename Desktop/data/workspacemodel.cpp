#include "workspacemodel.h"
#include "datasetpackage.h"
#include "workspace.h"
#include "qutils.h"
#include "gui/preferencesmodel.h"

WorkspaceModel* WorkspaceModel::_singleton = nullptr;

WorkspaceModel::WorkspaceModel(QObject *parent)
	: QObject(parent)
{
	if(_singleton) throw std::runtime_error("WorkspaceModel can be constructed only once!");

	_singleton = this;

	connect(DataSetPackage::pkg(),	&DataSetPackage::loadedChanged,					this,	&WorkspaceModel::refresh				);
	connect(Workspace::singleton(),	&Workspace::shownDataSetChanged,			this,	&WorkspaceModel::refresh				);
	connect(DataSetPackage::pkg(),	&DataSetPackage::nameChanged,					this,	&WorkspaceModel::nameChanged			);
	connect(DataSetPackage::pkg(),	&DataSetPackage::descriptionChanged,			this,	&WorkspaceModel::descriptionChanged		);
	connect(DataSetPackage::pkg(),	&DataSetPackage::workspaceEmptyValuesChanged,	this,	&WorkspaceModel::emptyValuesChanged		);
}

void WorkspaceModel::refresh()
{
	emit nameChanged();
	emit descriptionChanged();
	emit emptyValuesChanged();
}

QStringList WorkspaceModel::emptyValues() const
{
	return tql(DataSetPackage::pkg()->dataSet()->emptyValues()->emptyStrings());
}

QString WorkspaceModel::name() const
{
	return DataSetPackage::pkg()->name();
}

QString WorkspaceModel::description() const
{
	return DataSetPackage::pkg()->description();
}

void WorkspaceModel::setDescription(const QString &desc)
{
	if (desc == description()) return;

	UndoStack::singleton()->pushCommand(new SetWorkspacePropertyCommand(DataSetPackage::pkg()->dataSet(), desc, SetWorkspacePropertyCommand::WorkspaceProperty::Description));
}

void WorkspaceModel::removeEmptyValue(const QString &value)
{
	QStringList values = tql(DataSetPackage::pkg()->dataSet()->emptyValues()->emptyStrings());

	if (values.removeAll(value) > 0)
		UndoStack::singleton()->pushCommand(new SetWorkspaceEmptyValuesCommand(DataSetPackage::pkg()->dataSet(), values));
}

void WorkspaceModel::addEmptyValue(const QString &value)
{
	QStringList values = tql(DataSetPackage::pkg()->dataSet()->emptyValues()->emptyStrings());

	if (!values.contains(value))
	{
		values.push_back(value);
		UndoStack::singleton()->pushCommand(new SetWorkspaceEmptyValuesCommand(DataSetPackage::pkg()->dataSet(), values));
	}
}

void WorkspaceModel::resetEmptyValues()
{
	QStringList defaultValues = PreferencesModel::prefs()->emptyValues();

	if (defaultValues != emptyValues())
		UndoStack::singleton()->pushCommand(new SetWorkspaceEmptyValuesCommand(DataSetPackage::pkg()->dataSet(), defaultValues));
}
