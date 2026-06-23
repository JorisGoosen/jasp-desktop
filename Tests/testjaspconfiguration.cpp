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
#include "gui/jaspConfiguration/jaspconfiguration.h"
#include "gui/jaspConfiguration/jaspconfigurationtomlparser.h"

#include <QTest>
#include <QStringList>

class TestJASPConfiguration : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void cleanupTestCase();

	void testOverrideCommonEmpty();
	void testOverrideCommonSingle();
	void testOverrideCommonMultiple();
	void testOverrideCommonClear();
	void testOverrideCommonSetMultipleTimes();
	void testParseOverrideCommonFromTOML();
	void testParseOverrideCommonWithEnabledModules();
	void testParseOverrideCommonEmptyArray();
};

void TestJASPConfiguration::initTestCase()
{
	// Initialize test environment
}

void TestJASPConfiguration::cleanupTestCase()
{
	// Clean up test environment
	// Remove any singleton instance
	JASPConfiguration* config = JASPConfiguration::getInstance();
	if(config) {
		delete config;
		JASPConfiguration::_instance = nullptr;
	}
}

void TestJASPConfiguration::testOverrideCommonEmpty()
{
	JASPConfiguration config;

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(overrideCommon != nullptr);
	QVERIFY(overrideCommon->isEmpty());
}

void TestJASPConfiguration::testOverrideCommonSingle()
{
	JASPConfiguration config;

	config.setOverrideCommon(QStringList() << "jaspDescriptives");

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(!overrideCommon->isEmpty());
	QVERIFY(overrideCommon->size() == 1);
	QVERIFY(overrideCommon->contains("jaspDescriptives"));
}

void TestJASPConfiguration::testOverrideCommonMultiple()
{
	JASPConfiguration config;

	QStringList modules;
	modules << "jaspDescriptives" << "jaspTTests" << "jaspRegression";
	config.setOverrideCommon(modules);

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(!overrideCommon->isEmpty());
	QVERIFY(overrideCommon->size() == 3);
	QVERIFY(overrideCommon->contains("jaspDescriptives"));
	QVERIFY(overrideCommon->contains("jaspTTests"));
	QVERIFY(overrideCommon->contains("jaspRegression"));

	// Verify order is preserved
	QVERIFY(overrideCommon->at(0) == "jaspDescriptives");
	QVERIFY(overrideCommon->at(1) == "jaspTTests");
	QVERIFY(overrideCommon->at(2) == "jaspRegression");
}

void TestJASPConfiguration::testOverrideCommonClear()
{
	JASPConfiguration config;

	QStringList modules;
	modules << "jaspDescriptives" << "jaspTTests";
	config.setOverrideCommon(modules);

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(!overrideCommon->isEmpty());

	config.clear();
	overrideCommon = config.getOverrideCommon();
	QVERIFY(overrideCommon->isEmpty());
}

void TestJASPConfiguration::testOverrideCommonSetMultipleTimes()
{
	JASPConfiguration config;

	// First set
	QStringList modules1;
	modules1 << "jaspDescriptives";
	config.setOverrideCommon(modules1);

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(overrideCommon->size() == 1);
	QVERIFY(overrideCommon->contains("jaspDescriptives"));

	// Second set - should overwrite
	QStringList modules2;
	modules2 << "jaspTTests" << "jaspRegression";
	config.setOverrideCommon(modules2);

	overrideCommon = config.getOverrideCommon();
	QVERIFY(overrideCommon->size() == 2);
	QVERIFY(!overrideCommon->contains("jaspDescriptives"));
	QVERIFY(overrideCommon->contains("jaspTTests"));
	QVERIFY(overrideCommon->contains("jaspRegression"));
}

void TestJASPConfiguration::testParseOverrideCommonFromTOML()
{
	QString tomlData = R"(
		JASPVersion = "0.17.0"
		OverrideCommon = ["jaspDescriptives", "jaspTTests", "jaspRegression"]
	)";

	JASPConfiguration config;
	JASPConfigurationTOMLParser parser;
	bool result = parser.parse(&config, tomlData);

	QVERIFY(result);

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(!overrideCommon->isEmpty());
	QVERIFY(overrideCommon->size() == 3);
	QVERIFY(overrideCommon->at(0) == "jaspDescriptives");
	QVERIFY(overrideCommon->at(1) == "jaspTTests");
	QVERIFY(overrideCommon->at(2) == "jaspRegression");
}

void TestJASPConfiguration::testParseOverrideCommonWithEnabledModules()
{
	QString tomlData = R"(
		JASPVersion = "0.17.0"
		EnabledModules = ["jaspDescriptives", "jaspFrequencies"]
		OverrideCommon = ["jaspDescriptives", "jaspTTests"]
	)";

	JASPConfiguration config;
	JASPConfigurationTOMLParser parser;
	bool result = parser.parse(&config, tomlData);

	QVERIFY(result);

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(!overrideCommon->isEmpty());
	QVERIFY(overrideCommon->size() == 2);
	QVERIFY(overrideCommon->contains("jaspDescriptives"));
	QVERIFY(overrideCommon->contains("jaspTTests"));

	const QStringList* enabledModules = config.getAdditionalModules();
	QVERIFY(!enabledModules->isEmpty());
	QVERIFY(enabledModules->size() == 2);
	QVERIFY(enabledModules->contains("jaspDescriptives"));
	QVERIFY(enabledModules->contains("jaspFrequencies"));
}

void TestJASPConfiguration::testParseOverrideCommonEmptyArray()
{
	QString tomlData = R"(
		JASPVersion = "0.17.0"
		OverrideCommon = []
	)";

	JASPConfiguration config;
	JASPConfigurationTOMLParser parser;
	bool result = parser.parse(&config, tomlData);

	QVERIFY(result);

	const QStringList* overrideCommon = config.getOverrideCommon();
	QVERIFY(overrideCommon != nullptr);
	// Empty array should result in empty list
	QVERIFY(overrideCommon->isEmpty());
}

QTEST_MAIN(TestJASPConfiguration)
