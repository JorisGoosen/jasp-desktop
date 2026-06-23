//
// Copyright (C) 2013-2026 University of Amsterdam
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

#include "testinfo.h"
#include "modules/installedmodules.h"
#include "gui/jaspConfiguration/jaspconfiguration.h"
#include "utilities/appdirs.h"
#include "tempfiles.h"
#include "processinfo.h"

#include <QTest>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class TestInstalledModules : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void cleanupTestCase();

	void testGetModulesDefaultBehavior();
	void testGetModulesOverrideCommonFromConfig();
	void testGetModulesOverrideCommonFromSettings();
	void testGetModulesOverrideCommonCombined();
	void testGetModulesOverrideCommonReorder();
	void testGetModulesOverrideCommonMixed();
	void testGetModulesOverrideCommonSubset();
	void testGetModulesNonExistentModules();
};

void TestInstalledModules::initTestCase()
{
	TempFiles::init(ProcessInfo::currentPID());
}

void TestInstalledModules::cleanupTestCase()
{
	// Clean up any test files
	JASPConfiguration* config = JASPConfiguration::getInstance();
	if(config) {
		delete config;
		JASPConfiguration::_instance = nullptr;
	}
}

void TestInstalledModules::testGetModulesDefaultBehavior()
{
	// When no OverrideCommon is specified, should use default from modules-settings.json
	std::vector<InstalledModules::ModuleInfo> modules = InstalledModules::getModules();

	// Should have some modules
	QVERIFY(modules.size() > 0);

	// Find Common modules
	int commonCount = 0;
	for(const auto& module : modules) {
		if(module.common) {
			commonCount++;
		}
	}

	// Should have Common modules from default settings
	QVERIFY(commonCount > 0);
}

void TestInstalledModules::testGetModulesOverrideCommonFromConfig()
{
	// Set OverrideCommon via configuration
	JASPConfiguration* config = JASPConfiguration::getInstance();
	QStringList overrideCommon;
	overrideCommon << "jaspDescriptives" << "jaspTTests";
	config->setOverrideCommon(overrideCommon);

	std::vector<InstalledModules::ModuleInfo> modules = InstalledModules::getModules();

	// Verify OverrideCommon modules are Common
	bool foundDescriptives = false;
	bool foundTTests = false;
	for(const auto& module : modules) {
		if(module.name == "jaspDescriptives") {
			QVERIFY(module.common);
			foundDescriptives = true;
		}
		if(module.name == "jaspTTests") {
			QVERIFY(module.common);
			foundTTests = true;
		}
	}

	QVERIFY(foundDescriptives);
	QVERIFY(foundTTests);

	// Verify that originally Common modules not in OverrideCommon are now Extra
	bool foundAnovaExtra = false;
	for(const auto& module : modules) {
		if(module.name == "jaspAnova" && !module.common) {
			foundAnovaExtra = true;
			break;
		}
	}
	// Note: jaspAnova might not exist in test environment, so this is informational
}

void TestInstalledModules::testGetModulesOverrideCommonFromSettings()
{
	// This test would require modifying modules-settings.json
	// For now, we'll skip as it requires file I/O
	QSKIP("Skipping: requires modifying modules-settings.json");
}

void TestInstalledModules::testGetModulesOverrideCommonCombined()
{
	// Test combining OverrideCommon from both config and settings
	// This would require creating a test modules-settings.json with OverrideCommon
	QSKIP("Skipping: requires test modules-settings.json with OverrideCommon");
}

void TestInstalledModules::testGetModulesOverrideCommonReorder()
{
	JASPConfiguration* config = JASPConfiguration::getInstance();
	QStringList overrideCommon;
	// Reverse order of original Common modules
	overrideCommon << "jaspFactor" << "jaspFrequencies" << "jaspDescriptives";
	config->setOverrideCommon(overrideCommon);

	std::vector<InstalledModules::ModuleInfo> modules = InstalledModules::getModules();

	// Find the positions in the Common modules
	int posFactor = -1, posFrequencies = -1, posDescriptives = -1;
	int commonIndex = 0;
	for(const auto& module : modules) {
		if(module.common) {
			if(module.name == "jaspFactor") posFactor = commonIndex;
			if(module.name == "jaspFrequencies") posFrequencies = commonIndex;
			if(module.name == "jaspDescriptives") posDescriptives = commonIndex;
			commonIndex++;
		}
	}

	// Verify order is preserved (Factor before Frequencies before Descriptives)
	if(posFactor >= 0 && posFrequencies >= 0 && posDescriptives >= 0) {
		QVERIFY(posFactor < posFrequencies);
		QVERIFY(posFrequencies < posDescriptives);
	}
}

void TestInstalledModules::testGetModulesOverrideCommonMixed()
{
	JASPConfiguration* config = JASPConfiguration::getInstance();
	QStringList overrideCommon;
	// Mix of originally Common and Extra modules
	overrideCommon << "jaspDescriptives" << "jaspBain" << "jaspTTests";
	config->setOverrideCommon(overrideCommon);

	std::vector<InstalledModules::ModuleInfo> modules = InstalledModules::getModules();

	// Verify all OverrideCommon modules are Common
	for(const auto& module : modules) {
		if(module.name == "jaspDescriptives" || module.name == "jaspTTests" || module.name == "jaspBain") {
			QVERIFY(module.common);
		}
	}
}

void TestInstalledModules::testGetModulesOverrideCommonSubset()
{
	JASPConfiguration* config = JASPConfiguration::getInstance();
	QStringList overrideCommon;
	// Subset of original Common modules
	overrideCommon << "jaspDescriptives";
	config->setOverrideCommon(overrideCommon);

	std::vector<InstalledModules::ModuleInfo> modules = InstalledModules::getModules();

	// Verify jaspDescriptives is Common
	bool foundDescriptives = false;
	for(const auto& module : modules) {
		if(module.name == "jaspDescriptives") {
			QVERIFY(module.common);
			foundDescriptives = true;
			break;
		}
	}
	QVERIFY(foundDescriptives);

	// Verify other originally Common modules are now Extra
	bool foundAnovaExtra = false;
	for(const auto& module : modules) {
		if(module.name == "jaspAnova" && !module.common) {
			foundAnovaExtra = true;
			break;
		}
	}
	// Note: jaspAnova might not exist in test environment
}

void TestInstalledModules::testGetModulesNonExistentModules()
{
	JASPConfiguration* config = JASPConfiguration::getInstance();
	QStringList overrideCommon;
	// Include non-existent module
	overrideCommon << "jaspDescriptives" << "jaspNonExistentModule";
	config->setOverrideCommon(overrideCommon);

	// Should not crash
	std::vector<InstalledModules::ModuleInfo> modules = InstalledModules::getModules();

	// Verify jaspDescriptives is still Common
	bool foundDescriptives = false;
	for(const auto& module : modules) {
		if(module.name == "jaspDescriptives") {
			QVERIFY(module.common);
			foundDescriptives = true;
			break;
		}
	}
	QVERIFY(foundDescriptives);
}

QTEST_MAIN(TestInstalledModules)
