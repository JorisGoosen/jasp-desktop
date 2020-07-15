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

void RibbonButton::setDynamicModule(Modules::DynamicModule * module)
{
	_module = module;
	connect(_module, &Modules::DynamicModule::descriptionReloaded, this, &RibbonButton::reloadDynamicModule, Qt::QueuedConnection);
	_analysisMenuModel->setDynamicModule(_module);
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

RibbonButton::RibbonButton(QObject *parent,	std::string name, std::string title, std::string icon, bool requiresData, std::function<void ()> justThisFunction)
	: QObject(parent), _module(nullptr), _specialButtonFunc(justThisFunction)
{
	_analysisMenuModel = new AnalysisMenuModel(this, nullptr);
	setModuleName(name);
	setTitle(title);
	setIconSource(tq(icon));
	setRequiresData(requiresData);

	bindYourself();
}

void RibbonButton::bindYourself()
{
	connect(this,						&RibbonButton::enabledChanged,		this, &RibbonButton::somePropertyChanged);
	connect(this,						&RibbonButton::titleChanged,		this, &RibbonButton::somePropertyChanged);
	connect(this,						&RibbonButton::titleChanged,		this, &RibbonButton::somePropertyChanged);
	connect(this,						&RibbonButton::moduleNameChanged,	this, &RibbonButton::somePropertyChanged);
	connect(this,						&RibbonButton::dataLoadedChanged,	this, &RibbonButton::somePropertyChanged);
	connect(this,						&RibbonButton::requiresDataChanged,	this, &RibbonButton::somePropertyChanged);
	connect(this,						&RibbonButton::activeChanged,		this, &RibbonButton::somePropertyChanged);

	connect(this,						&RibbonButton::enabledChanged,		this, &RibbonButton::activeChanged		);
	connect(this,						&RibbonButton::dataLoadedChanged,	this, &RibbonButton::activeChanged		);
	connect(this,						&RibbonButton::requiresDataChanged,	this, &RibbonButton::activeChanged		);

	connect(DynamicModules::dynMods(),	&DynamicModules::dataLoadedChanged,	this, &RibbonButton::dataLoadedChanged	);
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

	if(DynamicModules::dynMods())
	{
		if(!isSpecial())
		{
			if(enabled)	DynamicModules::dynMods()->loadModule(moduleName());
			else		DynamicModules::dynMods()->unloadModule(moduleName());
		}

		emit DynamicModules::dynMods()->moduleEnabledChanged(moduleNameQ(), enabled);
	}
}

void RibbonButton::setIsCommon(bool isCommon)
{
	if (_isCommonModule == isCommon)
		return;

	_isCommonModule = isCommon;
	emit isCommonChanged();

	if(!_enabled && _isCommonModule)
		_enabled = true;
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
	return DynamicModules::dynMods()->dynamicModule(_moduleName);
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

/*
void RibbonButton::reloadMenuFromDescriptionQml()
{
	//Find the location of the module path
	QFileInfo modulepath = QFileInfo(QString::fromStdString(Dirs::resourcesDir() + _moduleName + "/"));
	if(!modulepath.exists())
	{
		Log::log() << "Path " << modulepath.absoluteFilePath().toStdString() << " does not exist!" << std::endl << std::flush;
		return;
	}

	QFileInfo descriptionFileInfo(modulepath.absoluteFilePath() + "/" + tq(Modules::DynamicModule::getQmlDescriptionFilename()));

	if(!descriptionFileInfo.exists())
	{
		Log::log() << "Could not find the qml description file : " << descriptionFileInfo.absoluteFilePath() << std::endl;
		return;
	}

	QFile descriptionFile(descriptionFileInfo.absoluteFilePath());
	if (!descriptionFile.open(QFile::ReadOnly))
	{
		Log::log() << "Could not open the qml description file : " << descriptionFile.fileName().toStdString()  << std::endl;
		return;
	}

	Modules::Description * desc = nullptr;

	try
	{
		desc = Modules::DynamicModule::instantiateDescriptionQml(descriptionFile.readAll(), QUrl::fromLocalFile(descriptionFileInfo.absoluteFilePath()), _moduleName);

		setRequiresData(		desc->requiresData());
		setTitle(			fq(	desc->title()	)	);
		setIconSource(			desc->icon()		);
		setToolTip(				desc->description() );

		setMenu(desc->menuEntries());


		if(_description)
			delete _description;

		_description = desc;

		connect(desc, &Modules::Description::iShouldBeUpdated, this, &RibbonButton::descriptionChanged);
	}
	catch(std::runtime_error e)
	{
		Log::log() << "(Re)loading " << Modules::DynamicModule::getQmlDescriptionFilename() << " had a problem: " << e.what() << std::endl;
	}
}

void RibbonButton::descriptionChanged(Modules::Description * desc)
{
	if(_description != desc)
	{
		Log::log() << "RibbonButton for module " << _moduleName << " was told to update from a Description but it isn't the one it knows about... Ignoring it!" << std::endl;
		return;
	}

	Log::log() << "RibbonButton for module " << _moduleName << " will read from Description again!" << std::endl;

	setRequiresData(		_description->requiresData());
	setTitle(			fq(	_description->title()	)	);
	setIconSource(			_description->icon()		);
	setMenu(				_description->menuEntries() );
	setToolTip(				_description->description()	);
}*/

void RibbonButton::setToolTip(QString toolTip)
{
	if (_toolTip == toolTip)
		return;

	_toolTip = toolTip;
	emit toolTipChanged(_toolTip);
}
