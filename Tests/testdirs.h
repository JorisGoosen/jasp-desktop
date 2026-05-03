#ifndef TESTDIRS_H
#define TESTDIRS_H

#include <QObject>
#include <QTest>

class TestDirs : public QObject
{
	Q_OBJECT

private slots:
	void testTempDir();
	void testExeDir();
	void testResourcesDir();
	void testSetAndGetReportingDir();
	void testSetAndGetLocalAppdataDir();
};

#endif // TESTDIRS_H
