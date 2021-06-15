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

#include "enginesync.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>


#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "jsonredirect.h"
#include "processinfo.h"
#include "common.h"
#include "appinfo.h"
#include "utilities/qutils.h"
#include "utils.h"
#include "tempfiles.h"
#include "timers.h"
#include "gui/preferencesmodel.h"
#include "utilities/appdirs.h"
#include "log.h"
#include "utilities/qutils.h"
#include "utilities/processhelper.h"


using namespace boost::interprocess;

EngineSync * EngineSync::_singleton = nullptr;

EngineSync::EngineSync(QObject *parent)
	: QObject(parent)
{
	assert(!_singleton);
	_singleton = this;
	
	using namespace Modules;

	connect(Analyses::analyses(),		&Analyses::sendRScript,								this,						&EngineSync::sendRCode							);
	connect(this,						&EngineSync::moduleInstallationFailed,				DynamicModules::dynMods(),	&DynamicModules::installationPackagesFailed		);
	connect(DynamicModules::dynMods(),	&DynamicModules::stopEngines,						this,						&EngineSync::stopEngines						);
	connect(DynamicModules::dynMods(),	&DynamicModules::restartEngines,					this,						&EngineSync::restartEngines						);
	connect(this,						&EngineSync::moduleInstallationSucceeded,			this,						&EngineSync::stopEngines						);
	connect(this,						&EngineSync::moduleInstallationSucceeded,			DynamicModules::dynMods(),	&DynamicModules::installationPackagesSucceeded,	Qt::QueuedConnection);
	connect(this,						&EngineSync::moduleInstallationSucceeded,			this,						&EngineSync::restartEngines,					Qt::QueuedConnection);

	connect(PreferencesModel::prefs(),	&PreferencesModel::plotPPIChanged,					this,						&EngineSync::settingsChanged					);
	connect(PreferencesModel::prefs(),	&PreferencesModel::plotBackgroundChanged,			this,						&EngineSync::settingsChanged					);
	connect(PreferencesModel::prefs(),	&PreferencesModel::languageCodeChanged,				this,						&EngineSync::settingsChanged					);
	connect(PreferencesModel::prefs(),	&PreferencesModel::developerModeChanged,			this,						&EngineSync::settingsChanged					);
	connect(PreferencesModel::prefs(),	&PreferencesModel::githubPatCustomChanged,			this,						&EngineSync::settingsChanged					);
	connect(PreferencesModel::prefs(),	&PreferencesModel::githubPatUseDefaultChanged,		this,						&EngineSync::settingsChanged					);

	// delay start so as not to increase program start up time 10sec is better than 100ms, because they are orphaned anyway
	// Except, that it might somehow cause a crash? If the timer goes off while waiting for a download from OSF than it might remove the files while making them..
	// So lets put it on 500ms...
	QTimer::singleShot(500, this, &EngineSync::deleteOrphanedTempFiles);

	DataSetPackage::pkg()->setEngineSync(this);

	_memoryName = "JASP-IPC-" + std::to_string(ProcessInfo::currentPID());
}

EngineSync::~EngineSync()
{
	if (_engineStarted)
	{		
		try			{ stopEngines(); }
		catch(...)	{ /* Whatever! */ }

		for(EngineRepresentation * engine : _engines)
			if(!engine->stopped())
				engine->killEngine();

		_moduleEngines.clear();
		_engines.clear();

		_rCmder  = nullptr;
		
		TempFiles::deleteAll();
	}
	_singleton = nullptr;
}

size_t EngineSync::maxEngineCount() const
{
	size_t maxEngines = std::max(1, PreferencesModel::prefs()->maxEngines());	
	return maxEngines;
}

void EngineSync::maxEngineCountChanged()
{
	Log::log() << "EngineSync::maxEngineCountChanged called and currently there are #" << _moduleEngines.size() << " while the max we want is: " << maxEngineCount() << std::endl;
	
	//We ignore the R-commander and utility engine for the max count
	while(_moduleEngines.size() > maxEngineCount())
	{
		//Pick first one by default
		EngineRepresentation * engineToKill = _moduleEngines.begin()->second;
		
		for(const auto & moduleEngine : _moduleEngines)
			if(moduleEngine.second->idle())
			{
				//But an idle one if possible
				engineToKill = moduleEngine.second;
				break;
			}
		
		engineToKill->shutEngineDown();
		destroyEngine(engineToKill);
	}
		
}

EngineRepresentation * EngineSync::createNewEngine(bool addToEngines)
{
	try
	{
		static size_t freeChannel = 0;
		EngineRepresentation * engine = new EngineRepresentation(new IPCChannel(_memoryName, freeChannel), startSlaveProcess(freeChannel), this);
		freeChannel++;
		
		if(addToEngines);
			_engines.insert(engine);

		connect(engine,						&EngineRepresentation::rCodeReturned,					Analyses::analyses(),	&Analyses::rCodeReturned												);
		connect(engine,						&EngineRepresentation::engineTerminated,				this,					&EngineSync::engineTerminated											);
		connect(engine,						&EngineRepresentation::processNewFilterResult,			this,					&EngineSync::processNewFilterResult										);
		connect(engine,						&EngineRepresentation::filterDone,						this,					&EngineSync::filterDone													);
		connect(engine,						&EngineRepresentation::processFilterErrorMsg,			this,					&EngineSync::processFilterErrorMsg										);
		connect(engine,						&EngineRepresentation::columnDataTypeChanged,			this,					&EngineSync::columnDataTypeChanged,				Qt::QueuedConnection	);
		connect(engine,						&EngineRepresentation::computeColumnSucceeded,			this,					&EngineSync::computeColumnSucceeded,			Qt::QueuedConnection	);
		connect(engine,						&EngineRepresentation::computeColumnFailed,				this,					&EngineSync::computeColumnFailed,				Qt::QueuedConnection	);
		connect(engine,						&EngineRepresentation::moduleLoadingFailed,				this,					&EngineSync::moduleLoadingFailedHandler									);
		connect(engine,						&EngineRepresentation::moduleLoadingSucceeded,			this,					&EngineSync::moduleLoadingSucceededHandler								);
		connect(engine,						&EngineRepresentation::moduleInstallationFailed,		this,					&EngineSync::moduleInstallationFailed									);
		connect(engine,						&EngineRepresentation::moduleInstallationSucceeded,		this,					&EngineSync::moduleInstallationSucceeded								);
		connect(engine,						&EngineRepresentation::moduleUninstallingFinished,		this,					&EngineSync::moduleUninstallingFinished									);
		connect(engine,						&EngineRepresentation::logCfgReplyReceived,				this,					&EngineSync::logCfgReplyReceived										);
		connect(engine,						&EngineRepresentation::plotEditorRefresh,				this,					&EngineSync::plotEditorRefresh											);
		connect(engine,						&EngineRepresentation::requestEngineRestart,			this,					&EngineSync::restartEngineAfterCrash									);
		connect(engine,						&EngineRepresentation::registerForModule,				this,					&EngineSync::registerEngineForModule									);
		connect(engine,						&EngineRepresentation::unregisterForModule,				this,					&EngineSync::unregisterEngineForModule									);
		connect(engine,						&EngineRepresentation::moduleHasEngine,					this,					&EngineSync::moduleHasEngine											);
		connect(this,						&EngineSync::settingsChanged,							engine,					&EngineRepresentation::settingsChanged									);
		connect(Analyses::analyses(),		&Analyses::analysisRemoved,								engine,					&EngineRepresentation::analysisRemoved									);
		connect(PreferencesModel::prefs(),	&PreferencesModel::maxEnginesChanged,					this,					&EngineSync::maxEngineCountChanged,				Qt::QueuedConnection	);


		return engine;

	}
	catch (interprocess_exception & e)
	{
		Log::log()  << "interprocess exception! " << e.what() <<  std::endl;
		throw e;
	}
}

void EngineSync::start(int )
{
	JASPTIMER_SCOPE(EngineSync::start);

	if (_engineStarted)
		return;

	_engineStarted = true;

	//We start with a single engine. Later we can start more if necessary and allowed by the user. This one engine can run filters etc and it can be assigned to a particular module.
	//Once it is assigned to a module it won't be possible to use it for another module until it is restarted.
	createNewEngine();

	QTimer	*timerProcess	= new QTimer(this),
			*timerBeat		= new QTimer(this);

	connect(timerProcess,	&QTimer::timeout, this, &EngineSync::process,				Qt::QueuedConnection);
	connect(timerBeat,		&QTimer::timeout, this, &EngineSync::heartbeatTempFiles,	Qt::QueuedConnection);

	timerProcess->start(50);
	timerBeat->start(30000);
}

void EngineSync::restartEngines()
{
	if(_engineStarted)
		return;

	Log::log() << "restarting engines!" << std::endl;

	for(auto * engine : _engines)
	{
		engine->restartEngine(startSlaveProcess(engine->channelNumber()));
		Log::log() << "restarted engine " << engine->channelNumber() << " but should still reload any active (dynamic) modules!"<< std::endl;
	}

	logCfgRequest();

	_engineStarted = true;
}

void EngineSync::restartEngineAfterCrash(EngineRepresentation * engine)
{
	engine->restartEngine(startSlaveProcess(engine->channelNumber()));
	logCfgRequest();
}

void EngineSync::restartKilledEngines()
{
	//Maybe we killed an engine because we wanted to pause or some option changed but the analysis wasn't listening. https://github.com/jasp-stats/INTERNAL-jasp/issues/875
	bool restartedAnEngine = false;

	for(EngineRepresentation * engine : _engines)
		if(engine->killed())
		{
			engine->restartEngine(startSlaveProcess(engine->channelNumber()));
			restartedAnEngine = true;
		}
}

void EngineSync::shutdownBoredEngines()
{
	std::vector<EngineRepresentation *> boredEngines;
	for (auto engine : _engines)
	{
		engine->processReplies();

		if(
			_engines.count(engine) > 0	&&
			engine->isBored()			&&
			_engines.size() - boredEngines.size()  > 1
		)
		{
		   Log::log() << "Engine #" << engine->channelNumber()  << " had nothing to do for so long it has decided to shutdown." << std::endl;
		   engine->shutEngineDown();
		   boredEngines.push_back(engine);
		}
	}

	for(EngineRepresentation * engine : boredEngines)
		destroyEngine(engine);
}

void EngineSync::process()
{
	restartKilledEngines();
	shutdownBoredEngines();
	processSettingsChanged();
	processFilterScript();

	if(_filterRunning) return; //Do not do anything else while waiting for a filter to return
	
	processLogCfgRequests();
	
	bool	notEnoughIdlesForScript		=	processScriptQueue(),
			notEnoughIdlesForModule		=	processDynamicModules(),
			notEnoughIdlesForAnalysis	=	processAnalysisRequests(),
			notEnoughIdles				=	notEnoughIdlesForScript || notEnoughIdlesForModule || notEnoughIdlesForAnalysis;

	if(notEnoughIdles)
		Log::log() << "Not enough idle engines! Need " << (notEnoughIdlesForScript ? " one for script " : "") << (notEnoughIdlesForModule ? " one for modules " : "") <<  (notEnoughIdlesForAnalysis ? " one for analysis" : "") << std::endl;
	
	if(notEnoughIdles && !anEngineIdleSoon())	
		startExtraEngine();
	else
		for (auto engine : _engines)
			if(engine->idle())
				engine->restartAbortedAnalysis();
}

int EngineSync::sendFilter(const QString & generatedFilter, const QString & filter)
{
	_waitingFilter = new RFilterStore(generatedFilter, filter, ++_filterCurrentRequestID);
	Log::log() << "waiting filter with requestid: " << _filterCurrentRequestID << " is now:\n" << generatedFilter.toStdString() << "\n" << filter.toStdString() << std::endl;

	return _filterCurrentRequestID;
}


void EngineSync::sendRCode(const QString & rCode, int requestId, bool whiteListedVersion)
{
	_waitingScripts.push(new RScriptStore(requestId, rCode, engineState::rCode, whiteListedVersion));
}

void EngineSync::computeColumn(const QString & columnName, const QString & computeCode, columnType colType)
{
	//first we remove the previously sent requests for this same column!
	std::queue<RScriptStore*> copiedWaiting(_waitingScripts);
	_waitingScripts = std::queue<RScriptStore*>() ;

	while(copiedWaiting.size() > 0)
	{
		RScriptStore * cur = copiedWaiting.front();
		if(cur->typeScript != engineState::computeColumn || static_cast<RComputeColumnStore*>(cur)->_columnName != columnName)
			_waitingScripts.push(cur);
		copiedWaiting.pop();
	}

	_waitingScripts.push(new RComputeColumnStore(columnName, computeCode, colType));
}

void EngineSync::processFilterScript()
{

	if (!_waitingFilter)
		return;

	Log::log() << "Pausing and resuming engines to make sure nothing else is running when we start the filter." << std::endl;

	//First we make sure nothing else is running before we ask the engine to run the filter
	if(!_filterRunning)
	{
		pauseEngines(); //Make sure engines pause/stop
		_filterRunning = true;
		resumeEngines();
	}
	else //So previous loop we made sure nothing else is running, and maybe we had to kill an engine to make it understand, followed by a restart. Now we are ready to run the filter
	{
		try
		{
			for (auto *engine : _engines)
				if (engine->idle()  && engine->runsUtility())
				{
					engine->runScriptOnProcess(_waitingFilter);
					_waitingFilter = nullptr;
					return;
				}

		} catch (...){	Log::log() << "Exception sent in processFilterScript" << std::endl;	}
	}
}

void EngineSync::filterDone(int requestID)
{
	if(requestID != _filterCurrentRequestID)
		return;

	_filterRunning = false; //Allow other stuff to happen
}

void EngineSync::processSettingsChanged()
{
	for(auto * engine : _engines)
		if(engine->shouldSendSettings())
			engine->sendSettings();
}

bool EngineSync::processScriptQueue()
{
	try
	{
		for(auto * engine : _engines)
		{
			if(engine->idle()  && engine->runsUtility() && _waitingScripts.size() > 0)
			{
				RScriptStore * waiting = _waitingScripts.front();

				switch(waiting->typeScript)
				{
				case engineState::rCode:			engine->runScriptOnProcess(waiting);						break;
				//case engineState::filter:			engine->runScriptOnProcess((RFilterStore*)waiting);			break;
				case engineState::computeColumn:	engine->runScriptOnProcess((RComputeColumnStore*)waiting);	break;
				default:							throw std::runtime_error("engineState " + engineStateToString(waiting->typeScript) + " unknown in EngineSync::processScriptQueue()!");
				}

				_waitingScripts.pop();
				delete waiting; //clean up
			}
		}
	}
	catch(...)
	{
		Log::log() << "Exception thrown in processScriptQueue" << std::endl;
	}
	
	return _waitingScripts.size() > 0; //Because if there are still scripts waiting we didnt find an idle engine for it
}


bool EngineSync::processDynamicModules()
{
	using namespace Modules;

	try
	{
		bool	wantToRunInstall	= DynamicModules::dynMods()->aModuleNeedsPackagesInstalled(),
				foundIdle			= false;
		
		if(!wantToRunInstall)
			return false;

		for(auto engine : _engines)
			if(engine->idle() && engine->runsUtility()) //We don't care if the engine is meant for some module or other. We restart afterwards anyway
			{
				foundIdle = true;
				engine->runModuleInstallRequestOnProcess(DynamicModules::dynMods()->getJsonForPackageInstallationRequest());
			}
		
		if(!foundIdle)
			return true;
	}
	catch(Modules::ModuleException & e)	{ Log::log() << "Exception thrown in processDynamicModules: " <<  e.what() << std::endl;	}
	catch(std::exception & e)			{ Log::log() << "Exception thrown in processDynamicModules: " << e.what() << std::endl;		}
	catch(...)							{ Log::log() << "Unknown Exception thrown in processDynamicModules..." << std::endl;		}
	
	return false;
}

bool EngineSync::processAnalysisRequests()
{	
	bool couldntFindIdleEngine = false;
	
	for(auto engine : _engines)
		engine->handleRunningAnalysisStatusChanges();

	Analyses::analyses()->applyToAll([&](Analysis * analysis)
	{
		if(analysis && analysis->shouldRun())
		{
			try
			{
				bool anEngineIsRunningMe = false;
				
				for(auto engine : _engines)
					if(engine->willProcessAnalysis(analysis))
					{
						engine->runAnalysisOnProcess(analysis);
						return;
					}
					else if (engine->analysisInProgress() == analysis)
						anEngineIsRunningMe = true;
					
				
				couldntFindIdleEngine = couldntFindIdleEngine || !anEngineIsRunningMe;
			}
			catch(...)	{ Log::log() << "Exception thrown in ProcessAnalysisRequests" << std::endl;	}
		}
	});
	
	return couldntFindIdleEngine;
}

///Maybe no engines are idle, but if one is initializing or setting up some stuff it'll be so soon. So tell JASP to be patient then.
bool EngineSync::anEngineIdleSoon()
{
	for(auto engine : _engines)
		if(engine->idleSoon())
			return true;
	return false;
}

void EngineSync::startExtraEngine()
{
	static int newEngineStartTime		= 0;
	if(maxEngineCount() > _engines.size() && newEngineStartTime + ENGINE_EXTRA_INTERVAL < Utils::currentSeconds())
	{
		newEngineStartTime = Utils::currentSeconds();
		Log::log() << "New engine requested, so starting it!" << std::endl;
		createNewEngine();
	}
}


#ifdef _WIN32 
///Overwrites the PATH with a simple clean one
void EngineSync::fixPATHForWindows(QProcessEnvironment & env)
{
	const QString R_ARCH =
#ifdef _WIN64
		"x64";
#else
		"i386";
#endif
	
	env.insert("PATH", AppDirs::programDir().absolutePath() + ";" + QDir(AppDirs::rHome()).absoluteFilePath("bin") + ";" + QDir(AppDirs::rHome()).absoluteFilePath("bin/" + R_ARCH)); // + rtoolsInPath); 

	Log::log() << "Windows PATH was changed to: '" << env.value("PATH", "???") << "'" << std::endl;
}
#endif

//Should this function go to EngineRepresentation?
QProcess * EngineSync::startSlaveProcess(int channel)
{
	JASPTIMER_SCOPE(EngineSync::startSlaveProcess);
	QDir programDir			= AppDirs::programDir();
	QString engineExe		= programDir.absoluteFilePath("JASPEngine");
	QProcessEnvironment env = ProcessHelper::getProcessEnvironmentForJaspEngine(true, PreferencesModel::prefs()->setLC_CTYPE_C());
	
#ifdef _WIN32 
	fixPATHForWindows(env);
#endif
	
	env.insert("GITHUB_PAT", PreferencesModel::prefs()->githubPatResolved());

	QStringList args;
	args << QString::number(channel) << QString::number(ProcessInfo::currentPID()) << QString::fromStdString(Log::logFileNameBase) << QString::fromStdString(Log::whereStr());

	QProcess *slave = new QProcess(this);
	slave->setProcessChannelMode(QProcess::ForwardedChannels);
	slave->setProcessEnvironment(env);
	slave->setWorkingDirectory(QFileInfo( QCoreApplication::applicationFilePath() ).absoluteDir().absolutePath());

#ifdef _WIN32
	/*
	On Windows, QProcess uses the Win32 API function CreateProcess to
	start child processes.In some casedesirable to fine-tune
	the parameters that are passed to CreateProcess.
	This is done by defining a CreateProcessArgumentModifier function and passing it
	to setCreateProcessArgumentsModifier

	bInheritHandles [in]
	If this parameter is TRUE, each inheritable handle in the calling process
	is inherited by the new process. If the parameter is FALSE, the handles
	are not inherited.
	*/

	slave->setCreateProcessArgumentsModifier([] (QProcess::CreateProcessArguments *args)
	{
#ifndef QT_DEBUG
		args->inheritHandles = false;
#endif
	});
#endif

	slave->start(engineExe, args);

	return slave;
}

void EngineSync::deleteOrphanedTempFiles()
{
	TempFiles::deleteOrphans();
}

void EngineSync::heartbeatTempFiles()
{
	TempFiles::heartbeat();
}

void EngineSync::stopEngines()
{
	if(!_engineStarted) return;

	auto timeout = QDateTime::currentSecsSinceEpoch() + 10;

	//make sure we process any received messages first.
	for(auto engine : _engines)
		engine->processReplies();

	_engineStarted		= false;

	for(EngineRepresentation * e : _engines)
		e->stopEngine();

	while(!allEnginesStopped())
		if(timeout < QDateTime::currentSecsSinceEpoch())
		{
			Log::log() << "Waiting for engine to reply stopRequest took longer than timeout, killing it/them.." << std::endl;
			for(EngineRepresentation * e : _engines)
				if(!e->stopped() && !e->killed())
					e->killEngine();

			break;
		}
		else
			for (auto * engine : _engines)
				engine->processReplies();

	//timeout = QDateTime::currentSecsSinceEpoch() + 10;

	/*
		This is all commented out because it is very dangerous to just go and processEvents in the middle of other functions...
	  
	  Log::log() << "Checking if engines are running by using QApplication::processEvents to get answers." << std::endl;

	bool stillRunning;
	do
	{
		QApplication::processEvents(); //Otherwise we will not get feedback from QProcess (aka finished)

		stillRunning = false;

		for (auto * engine : _engines)
			if(engine->jaspEngineStillRunning())
				stillRunning = true;
	}
	while(stillRunning && timeout > QDateTime::currentSecsSinceEpoch()); //Let's give good old jaspEngines some time to shutdown gracefully

	if(stillRunning)
	{
		Log::log() << "Waiting for engine to stop took longer than timeout, so we'll just kill them/it." << std::endl;

		for (auto * engine : _engines)
			if(engine->jaspEngineStillRunning())
				engine->killEngine();
	}*/

	Log::log() << "Engines stopped(/killed)" << std::endl;
}

void EngineSync::pauseEngines()
{
	JASPTIMER_SCOPE(EngineSync::pauseEngines);

	if(!_engineStarted) return;

	//make sure we process any received messages first.
	for(auto engine : _engines)
		engine->processReplies();

	for(EngineRepresentation * e : _engines)
		e->pauseEngine();

	long tryTill = Utils::currentSeconds() + ENGINE_KILLTIME; //Ill give the engine 1 sec to respond

	while(!allEnginesPaused() && tryTill >= Utils::currentSeconds())
		for (auto * engine : _engines)
			engine->processReplies();

	for (auto * engine : _engines)
		if(!engine->paused())
			engine->killEngine();
}

void EngineSync::resumeEngines()
{
	JASPTIMER_SCOPE(EngineSync::resumeEngines);

	if(!_engineStarted)
		return;

	bool restartedAnEngine = false;

	for(EngineRepresentation * engine : _engines)
		if(!engine->jaspEngineStillRunning())
		{
			engine->restartEngine(startSlaveProcess(engine->channelNumber()));
			restartedAnEngine = true;
		}
		else
			engine->resumeEngine();

	while(!allEnginesResumed())
		for (auto * engine : _engines)
			engine->processReplies();
}

bool EngineSync::allEnginesStopped()
{
	for(auto * engine : _engines)
		if(!engine->stopped())
			return false;
	return true;
}

bool EngineSync::allEnginesPaused()
{
	for(auto * engine : _engines)
		if(!engine->paused()) //Initializing() is part paused()
			return false;
	return true;
}

bool EngineSync::allEnginesResumed()
{
	for(auto * engine : _engines)
		if(!engine->resumed())
			return false;
	return true;
}

bool EngineSync::allEnginesInitializing()
{
	for(auto * engine : _engines)
		if(!engine->initializing())
			return false;
	return true;
}

void EngineSync::moduleLoadingFailedHandler(const QString & moduleName, const QString & errorMessage, int channelID)
{
	Log::log() << "Received EngineSync::moduleLoadingFailedHandler(" << moduleName.toStdString() << ", " << errorMessage.toStdString() << ", " << channelID << ")" << std::endl;

	throw std::runtime_error("EngineSync::moduleLoadingFailedHandler should probably have some functionality dont you think?");
}

void EngineSync::moduleLoadingSucceededHandler(const QString & moduleName, int channelID)
{
	Log::log() << "Received EngineSync::moduleLoadingSucceededHandler(" << moduleName.toStdString() << ", " << channelID << ")" << std::endl;

	throw std::runtime_error("EngineSync::moduleLoadingSucceededHandler should probably have some functionality dont you think?");
}


void EngineSync::refreshAllPlots()
{
	std::set<Analysis*> inProgress;
	for(EngineRepresentation * engine : _engines)
		if(engine->analysisInProgress() != nullptr)
			inProgress.insert(engine->analysisInProgress());

	//If an analysis is empty it means it will be reran anyway, so rewriteImgs is pointless
	Analyses::analyses()->applyToAll([&](Analysis * analysis)
	{
		if(analysis->isEmpty())
			inProgress.insert(analysis);
	});

	emit refreshAllPlotsExcept(inProgress);
}


void EngineSync::logCfgRequest()
{
	for(EngineRepresentation * e : _engines)
		_logCfgRequested.insert(e);
}

void EngineSync::logCfgReplyReceived(EngineRepresentation * engine)
{
	_logCfgRequested.erase(engine);
}

void EngineSync::registerEngineForModule(EngineRepresentation * engine, std::string modName)
{
	if(_moduleEngines.count(modName) > 0 && _moduleEngines[modName] != engine)
		throw std::runtime_error("Trying to register module '" + modName + "' to engine #" +
								 std::to_string(engine->channelNumber()) + " but it is already registered to " +
								 std::to_string(_moduleEngines[modName]->channelNumber()));

	_moduleEngines[modName] = engine;
}

void EngineSync::unregisterEngineForModule(EngineRepresentation * engine, std::string modName)
{
	if(_moduleEngines.count(modName) > 0 && _moduleEngines[modName] != engine)
		return;

	_moduleEngines.erase(modName); //We only erase it when it is the exact same engine + modName combo
}

void EngineSync::killModuleEngine(Modules::DynamicModule * mod)
{
	if(!_moduleEngines.count(mod->name()))
		return;

	destroyEngine(_moduleEngines[mod->name()]);
}

void EngineSync::processLogCfgRequests()
{
	if (_logCfgRequested.size() == 0)
		return;

	try
	{
		for(auto * engine : _logCfgRequested)
			if(engine->idle())
				engine->sendLogCfg();
	}
	catch (...)
	{
		Log::log() << "Exception thrown in processLogCfgRequests" << std:: endl << std::flush;
	}
}

void EngineSync::cleanUpAfterClose()
{
	//try { stopEngines(); } //Tends to go wrong when the engine was already killed (for instance because it didnt want to pause)
	try {	pauseEngines(); }
	catch(unexpectedEngineReply e) {} // If we are cleaning up after close we can get all sorts of things, lets just ignore them.

	while(_waitingScripts.size() > 0)
	{
		delete _waitingScripts.front();
		_waitingScripts.pop();
	}

	if(_waitingFilter)
		delete _waitingFilter;
	_waitingFilter = nullptr;

	TempFiles::clearSessionDir();

	for(EngineRepresentation * e : _engines)
		e->cleanUpAfterClose();

	try { resumeEngines(); }
	//try { restartEngines(); }
	catch(unexpectedEngineReply e) {}


}

std::string	EngineSync::currentStateForDebug() const
{
	try
	{
		std::stringstream out;

		for(auto * engine : _engines)
			try			{ out << engine->currentStateForDebug() << "\n"; }
			catch(...)	{ out << "Something is wrong with engine " << engine->channelNumber() << "...\n"; }

		return out.str();
	}
	catch(...)
	{
		return "EngineSync::currentStateForDebug() did not work...\n";
	}

}

EngineRepresentation *	EngineSync::createRCmdEngine()
{
	if(!_rCmder)
	{

		_rCmder = createNewEngine(false);


		_rCmder->setRunsAnalysis(	false);
		_rCmder->setRunsUtility(	false);
		_rCmder->setRunsRCmd(		true);
	}

	return _rCmder;
}

void EngineSync::destroyEngine(EngineRepresentation * engine)
{
	if(!engine) return;

	if(_rCmder  == engine)	_rCmder  = nullptr;
	
	_moduleEngines.erase(engine->dynamicModule());

	_engines.erase(engine);

	delete engine;
}
