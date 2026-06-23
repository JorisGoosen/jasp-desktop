#ifndef TESTINSTALLEDMODULES_H
#define TESTINSTALLEDMODULES_H

#include <QObject>

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

#endif // TESTINSTALLEDMODULES_H
