//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//


#include "ribbonmodel.h"
#include "dirs.h"
#include "log.h"

RibbonModel::RibbonModel(DynamicModules * dynamicModules, PreferencesModel * preferences)
	: QAbstractListModel(dynamicModules), _dynamicModules(dynamicModules), _preferences(preferences)
{
	connect(_dynamicModules, &DynamicModules::dynamicModuleAdded,		this, &RibbonModel::addDynamicRibbonButtonModel		);
	connect(_dynamicModules, &DynamicModules::dynamicModuleUninstalled,	this, &RibbonModel::removeDynamicRibbonButtonModel	);
	connect(_dynamicModules, &DynamicModules::dynamicModuleReplaced,	this, &RibbonModel::dynamicModuleReplaced			);
	connect(_dynamicModules, &DynamicModules::dynamicModuleChanged,		this, &RibbonModel::dynamicModuleChanged			);


}

void RibbonModel::loadModules(std::vector<std::string> commonModulesToLoad, std::vector<std::string> extraModulesToLoad)
{
	for(const std::string & moduleName : commonModulesToLoad)
		if(_dynamicModules->moduleIsInstalledInItsOwnLibrary(moduleName)) //Only load bundled if the user did not install a newer/other version
			addRibbonButtonModelFromDynamicModule((*_dynamicModules)[moduleName]);
		else
		{
			std::string moduleLibrary = Dirs::bundledDir() + moduleName + "/" ;
			_dynamicModules->initializeModuleFromDir(moduleLibrary, true, true);
		}

	for(const std::string & moduleName : extraModulesToLoad)
		if(_dynamicModules->moduleIsInstalledInItsOwnLibrary(moduleName)) //Only load bundled if the user did not install a newer/other version
			addRibbonButtonModelFromDynamicModule((*_dynamicModules)[moduleName]);
		else
		{
			std::string moduleLibrary = Dirs::bundledDir() + moduleName + "/" ;
			_dynamicModules->initializeModuleFromDir(moduleLibrary, true);
		}

	for(const std::string & modName : _dynamicModules->moduleNames())
		if(!isModuleName(modName)) //Was it already added from commonModulesToLoad or extraModulesToLoad?
			addRibbonButtonModelFromDynamicModule((*_dynamicModules)[modName]);

	if(_preferences->modulesRemember())
	{
		QStringList enabledModules = _preferences->modulesRemembered();

		for(const QString & enabledModule : enabledModules)
		{
			std::string mod = enabledModule.toStdString();

			if(_buttonModelsByName.count(mod) > 0)
				_buttonModelsByName[mod]->setEnabled(true);
		}
	}
}

void RibbonModel::addRibbonButtonModelFromDynamicModule(Modules::DynamicModule * module)
{
	RibbonButton * button = new RibbonButton(this, module);

	addRibbonButtonModel(button);
}

void RibbonModel::dynamicModuleChanged(Modules::DynamicModule * dynMod)
{
	Log::log() << "void RibbonModel::dynamicModuleChanged(" << dynMod->toString() << ")" << std::endl;

	for(const auto & nameButton : _buttonModelsByName)
		if(nameButton.second->dynamicModule() == dynMod)
			nameButton.second->reloadDynamicModule(dynMod);
}

void RibbonModel::addRibbonButtonModel(RibbonButton* model)
{
	model->setDynamicModules(_dynamicModules);

	if(isModuleName(model->moduleName()))
		removeRibbonButtonModel(model->moduleName());

	emit beginInsertRows(QModelIndex(), rowCount(), rowCount());

	_moduleNames.push_back(model->moduleName());
	_buttonModelsByName[model->moduleName()] = model;

	emit endInsertRows();

	connect(model, &RibbonButton::iChanged, this, &RibbonModel::ribbonButtonModelChanged);
}

void RibbonModel::dynamicModuleReplaced(Modules::DynamicModule * oldModule, Modules::DynamicModule * module)
{
	for(const auto & nameButton : _buttonModelsByName)
		if(nameButton.second->dynamicModule() == oldModule || nameButton.first == oldModule->name())
			nameButton.second->reloadDynamicModule(module);
}

QVariant RibbonModel::data(const QModelIndex &index, int role) const
{
	if (index.row() >= rowCount())
		return QVariant();

	size_t row = size_t(index.row());

	switch(role)
	{
	case DisplayRole:		return ribbonButtonModelAt(row)->titleQ();
	case RibbonRole:		return QVariant::fromValue(ribbonButtonModelAt(row));
	case EnabledRole:		return ribbonButtonModelAt(row)->enabled();
	case ActiveRole:		return ribbonButtonModelAt(row)->active();
	case CommonRole:		return ribbonButtonModelAt(row)->isCommon();
	case ModuleNameRole:	return ribbonButtonModelAt(row)->moduleNameQ();
	case ModuleTitleRole:	return ribbonButtonModelAt(row)->titleQ();
	case ModuleRole:		return QVariant::fromValue(ribbonButtonModelAt(row)->dynamicModule());
	case BundledRole:		return ribbonButtonModelAt(row)->isBundled();
	case VersionRole:		return ribbonButtonModelAt(row)->version();
	case ClusterRole:		//To Do!?
	default:				return QVariant();
	}
}


QHash<int, QByteArray> RibbonModel::roleNames() const
{
	static const auto roles = QHash<int, QByteArray>{
		{ ClusterRole,		"clusterMenu"		},
		{ DisplayRole,		"displayText"		},
		{ RibbonRole,		"ribbonButton"		},
		{ EnabledRole,		"ribbonEnabled"		},
		{ CommonRole,		"isCommon"			},
		{ ModuleNameRole,	"moduleName"		},
		{ ModuleTitleRole,	"moduleTitle"		},
		{ ModuleRole,		"dynamicModule"		},
		{ ActiveRole,		"active"			},
		{ BundledRole,		"isBundled"			},
		{ VersionRole,		"moduleVersion"		}
	};

	return roles;
}

RibbonButton* RibbonModel::ribbonButtonModel(std::string name) const
{
	if(_buttonModelsByName.count(name) > 0)
		return _buttonModelsByName.at(name);

	return nullptr;
}

void RibbonModel::removeRibbonButtonModel(std::string moduleName)
{
	if(!isModuleName(moduleName))
		return;

	int indexRemoved = -1;

	for(int i=_moduleNames.size() - 1; i >= 0; i--)
		if(_moduleNames[i] == moduleName)
		{
			indexRemoved = i;
			break;
		}

	emit beginRemoveRows(QModelIndex(), indexRemoved, indexRemoved);

	delete _buttonModelsByName[moduleName];
	_buttonModelsByName.erase(moduleName);

	_moduleNames.erase(_moduleNames.begin() + indexRemoved);

	emit endRemoveRows();
}

void RibbonModel::setHighlightedModuleIndex(int highlightedModuleIndex)
{
	if (_highlightedModuleIndex == highlightedModuleIndex)
		return;

	_highlightedModuleIndex = highlightedModuleIndex;
	emit highlightedModuleIndexChanged(_highlightedModuleIndex);
}

void RibbonModel::setModuleEnabled(int ribbonButtonModelIndex, bool enabled)
{
	if(ribbonButtonModelIndex < 0)
		return;

	RibbonButton * ribbonButtonModel = ribbonButtonModelAt(size_t(ribbonButtonModelIndex));

	if(ribbonButtonModel->enabled() != enabled)
	{
		ribbonButtonModel->setEnabled(enabled);
		emit dataChanged(index(ribbonButtonModelIndex), index(ribbonButtonModelIndex));
	}
}

Modules::AnalysisEntry *RibbonModel::getAnalysis(const std::string& moduleName, const std::string& analysisName)
{
	Modules::AnalysisEntry* analysis = nullptr;
	RibbonButton* ribbonButton = ribbonButtonModel(moduleName);
	if (ribbonButton)
		analysis = ribbonButton->getAnalysis(analysisName);

	return analysis;
}

QString RibbonModel::getModuleNameFromAnalysisName(const QString analysisName)
{
	QString result = "Common";
	std::string searchName = analysisName.toStdString();
	// This function is needed for old JASP file: they still have a reference to the common mondule that does not exist anymore.
	for (const std::string& myModuleName : _moduleNames)
	{
		RibbonButton* button = _buttonModelsByName[myModuleName];
		for (const std::string& name : button->getAllAnalysisNames())
		{
			if (name == searchName)
			{
				result = QString::fromStdString(myModuleName);
				break;
			}
		}
		if (result != "Common")
			break;

	}

	return result;
}

void RibbonModel::toggleModuleEnabled(int ribbonButtonModelIndex)
{
	if(ribbonButtonModelIndex < 0)
		return;

	RibbonButton * ribbonButtonModel = ribbonButtonModelAt(size_t(ribbonButtonModelIndex));

	ribbonButtonModel->setEnabled(!ribbonButtonModel->enabled());

	emit dataChanged(index(ribbonButtonModelIndex), index(ribbonButtonModelIndex));
}

int RibbonModel::ribbonButtonModelIndex(RibbonButton * model)	const
{
	for(auto & keyval : _buttonModelsByName)
		if(keyval.second == model)
			for(size_t i=0; i<_moduleNames.size(); i++)
				if(_moduleNames[i] == keyval.first)
					return int(i);
	return -1;
}


void RibbonModel::ribbonButtonModelChanged(RibbonButton* model)
{
	int row = ribbonButtonModelIndex(model);
	emit dataChanged(index(row), index(row));
}

void RibbonModel::moduleLoadingSucceeded(const QString & moduleName)
{
	if(moduleName == "*")
		return;

	RibbonButton * ribMod = ribbonButtonModel(moduleName.toStdString());
	ribMod->setEnabled(true);
}
