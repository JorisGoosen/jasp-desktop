#ifndef TESTJASPCONFIGURATION_H
#define TESTJASPCONFIGURATION_H

#include <QObject>

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

#endif // TESTJASPCONFIGURATION_H
