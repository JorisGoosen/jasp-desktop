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


#include "ribbonbutton.h"
#include "enginedefinitions.h"
#include "modules/dynamicmodule.h"
#include "modules/analysisentry.h"
#include "utilities/qutils.h"
#include "log.h"

RibbonButton::RibbonButton(QObject *parent, Modules::DynamicModule * module)  : QObject(parent), _module(module)
{
	setTitle(			_module->title()					);
	setRequiresData(	_module->requiresData()				);
	setIsCommon(		_module->isCommon()					);
	setModuleName(		_module->name()						);
	setIconSource(tq(	_module->iconFilePath())			);

	bindYourself();

	_analysisMenuModel = new AnalysisMenuModel(this, _module);

	setDynamicModule(_module);
}


void RibbonButton::reloadDynamicModule(Modules::DynamicModule * dynMod)
{
	bool dynamicModuleChanged = _module != dynMod;

	if(dynamicModuleChanged)
		setDynamicModule(dynMod);

	setTitle(			_module->title()		);
	setRequiresData(	_module->requiresData()	);
	setIconSource(tq(	_module->iconFilePath()));

	//if(dynamicModuleChanged)
	emit iChanged(this);
}

void RibbonButton::setDynamicModule(Modules::DynamicModule * module)
{
	_module = module;
	connect(_module, &Modules::DynamicModule::descriptionReloaded, this, &RibbonButton::reloadDynamicModule, Qt::QueuedConnection);
	_analysisMenuModel->setDynamicModule(_module);
}

void RibbonButton::bindYourself()
{
	connect(this, &RibbonButton::enabledChanged,		this, &RibbonButton::somePropertyChanged);
	connect(this, &RibbonButton::titleChanged,			this, &RibbonButton::somePropertyChanged);
	connect(this, &RibbonButton::titleChanged,			this, &RibbonButton::somePropertyChanged);
	connect(this, &RibbonButton::moduleNameChanged,		this, &RibbonButton::somePropertyChanged);
	connect(this, &RibbonButton::dataLoadedChanged,		this, &RibbonButton::somePropertyChanged);
	connect(this, &RibbonButton::requiresDataChanged,	this, &RibbonButton::somePropertyChanged);
	connect(this, &RibbonButton::activeChanged,			this, &RibbonButton::somePropertyChanged);

	connect(this, &RibbonButton::enabledChanged,		this, &RibbonButton::activeChanged);
	connect(this, &RibbonButton::dataLoadedChanged,		this, &RibbonButton::activeChanged);
	connect(this, &RibbonButton::requiresDataChanged,	this, &RibbonButton::activeChanged);
}

void RibbonButton::setRequiresData(bool requiresDataset)
{
	if(_requiresData == requiresDataset)
		return;

	_requiresData = requiresDataset;
	emit requiresDataChanged();
}

void RibbonButton::setTitle(std::string title)
{
	if(_title == title)
		return;

	_title = title;
	emit titleChanged();
}

void RibbonButton::setIconSource(QString iconSource)
{
	Log::log() << "Iconsource ribbonbutton changed to: " << iconSource.toStdString() << std::endl;

	_iconSource = iconSource;
	emit iconSourceChanged();
}

void RibbonButton::setEnabled(bool enabled)
{
	if (_enabled == enabled)
		return;

	_enabled = enabled;
	emit enabledChanged();

	if(_dynamicModules != nullptr)
	{
		if(enabled)	_dynamicModules->loadModule(moduleName());
		else		_dynamicModules->unloadModule(moduleName());

		emit _dynamicModules->moduleEnabledChanged(moduleNameQ(), enabled);
	}
}

void RibbonButton::setIsCommon(bool isCommon)
{
	if (_isCommonModule == isCommon)
		return;

	_isCommonModule = isCommon;
	emit isCommonChanged();
}

void RibbonButton::setModuleName(std::string moduleName)
{
	if (_moduleName == moduleName)
		return;

	_moduleName = moduleName;
	emit moduleNameChanged();
}

Modules::DynamicModule * RibbonButton::dynamicModule()
{
	return _dynamicModules->dynamicModule(_moduleName);
}

Modules::AnalysisEntry *RibbonButton::getAnalysis(const std::string &name)
{
	Modules::AnalysisEntry* analysis = nullptr;
	analysis = _analysisMenuModel->getAnalysisEntry(name);
	
	return analysis;
}

std::vector<std::string> RibbonButton::getAllAnalysisNames() const
{
	std::vector<std::string> allAnalyses;
	for (Modules::AnalysisEntry* menuEntry : _module->menu())
		if (menuEntry->isAnalysis())
			allAnalyses.push_back(menuEntry->function());

	return allAnalyses;
}

void RibbonButton::setDynamicModules(DynamicModules * dynamicModules)
{
	_dynamicModules = dynamicModules;
	connect(_dynamicModules, &DynamicModules::dataLoadedChanged, this, &RibbonButton::dataLoadedChanged);
}
