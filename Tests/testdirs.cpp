#include "testdirs.h"
#include "dirs.h"
#include <QDir>

void TestDirs::testTempDir()
{
	std::string tempDir = Dirs::tempDir();
	
	QVERIFY2(!tempDir.empty(), "Temp directory should not be empty");
	
	// Check it's a valid directory
	QDir dir(QString::fromStdString(tempDir));
	QVERIFY2(dir.exists(), "Temp directory should exist");
}

void TestDirs::testExeDir()
{
	std::string exeDir = Dirs::exeDir();
	
	QVERIFY2(!exeDir.empty(), "Exe directory should not be empty");
}

void TestDirs::testResourcesDir()
{
	std::string resDir = Dirs::resourcesDir();
	
	QVERIFY2(!resDir.empty(), "Resources directory should not be empty");
}

void TestDirs::testSetAndGetReportingDir()
{
	std::string testDir = "/tmp/test_reporting_dir";
	
	Dirs::setReportingDir(testDir);
	
	std::string retrieved = Dirs::reportingDir();
	
	QVERIFY2(retrieved == testDir, "Reporting dir should match what was set");
}

void TestDirs::testSetAndGetLocalAppdataDir()
{
	std::string testDir = "/tmp/test_localappdata_dir";
	
	Dirs::setLocalAppdataDir(testDir);
	
	std::string retrieved = Dirs::localAppDataDir();
	
	QVERIFY2(retrieved == testDir, "Local appdata dir should match what was set");
}

QTEST_MAIN(TestDirs)
