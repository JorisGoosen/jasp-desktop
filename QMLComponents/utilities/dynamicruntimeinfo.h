#ifndef DYNRUNTIMEINFO_H
#define DYNRUNTIMEINFO_H

/*! 
 *  Simple class that parses Runtime information from staticRuntimeInfo.json located in the install folder
 *  and dynamicRuntimeInfo.json located in a user directory defined by Appdirs. 
 *  staticRuntimeInfo.json containts information on install type. 
 * 	dynamicRuntimeInfo.json is used to log various information regarding the initialization of the environment. e.g. at first run
 * 	This class is used to query runtime information and to determine if there has been proper initialization for this JASP version.
*/

#include <cstdint>
#include <string>
#include <map>
#include "dynamicruntimeenums.h"

class DynamicRuntimeInfo 
{
protected:
	DynamicRuntimeInfo();

public:
	static DynamicRuntimeInfo * getInstance();
								DynamicRuntimeInfo(DynamicRuntimeInfo& other)	= delete;
	void						operator=(const DynamicRuntimeInfo&)			= delete;

	bool						bundledModulesInitialized();
	RuntimeEnvironment			runtimeEnvironment()						{ return _environment;					}
	MicroArch					microArch()									{ return _arch;							}
	uint64_t					bundledModulesInitializedOnTimestamp()		{ return _initializedOn;				}
	std::string					bundledModulesInitializedByCommit()			{ return _initializedByCommit;			}
	std::string					bundledModulesInitializedByBuildDate()		{ return _initializedByBuildDate;		}
	std::string					bundledModulesInitializedRVersion()			{ return _initializedForRVersion;		}
	std::string					bundledModulesInitializedJaspVersion()		{ return _initializedForJaspVersion;	}

	bool						writeDynamicRuntimeInfoFile();

protected:
	bool						parseStaticRuntimeInfoFile	(const std::string & path);
	bool						parseDynamicRuntimeInfoFile	(const std::string & path);

private:
	std::string					staticRuntimeInfoFilePath();
	std::string					dynamicRuntimeInfoFilePath();

	RuntimeEnvironment			_environment;
	MicroArch					_arch;

	bool						_bundledModulesInitializedSet	= true;
	std::string					_initializedByCommit			= "build",
								_initializedForRVersion			= "build",
								_initializedByBuildDate			= "build",
								_initializedForJaspVersion		= "build";
	uint64_t					_initializedOn					= 0;

	static DynamicRuntimeInfo * _instance;

	static const std::string	staticInfoFileName;
	static const std::string	dynamicInfoFileName;
};

#endif // DYNRUNTIMEINFO_H
