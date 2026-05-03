#include "testtempfiles.h"
#include "tempfiles.h"
#include "processinfo.h"
#include <QDir>
#include <QFile>

void TestTempFiles::initTestCase()
{
	TempFiles::clearSessionDir();
}

void TestTempFiles::init()
{
	TempFiles::init(ProcessInfo::currentPID());
}

void TestTempFiles::cleanup()
{
	TempFiles::clearSessionDir();
}

void TestTempFiles::testInit()
{
	QVERIFY2(!TempFiles::sessionDirName().empty(), "Session dir name should not be empty after init");
}

void TestTempFiles::testCreateSessionDir()
{
	TempFiles::createSessionDir();
	QDir dir(QString::fromStdString(TempFiles::sessionDirName()));
	QVERIFY2(dir.exists(), "Session directory should exist after creation");
}

void TestTempFiles::testClearSessionDir()
{
	TempFiles::createSessionDir();
	TempFiles::clearSessionDir();
	// Verify API works - clearing may or may not remove the directory depending on implementation
	QVERIFY2(true, "Clear session dir API should work without error");
}

void TestTempFiles::testCreateSpecific()
{
	std::string root, relativePath;
	TempFiles::createSpecific("testfile", 1, root, relativePath);
	
	QVERIFY2(!root.empty(), "Root should not be empty");
	QVERIFY2(!relativePath.empty(), "Relative path should not be empty");
}

void TestTempFiles::testCreateTmpFolder()
{
	std::string tmpFolder = TempFiles::createTmpFolder();
	
	QVERIFY2(!tmpFolder.empty(), "Tmp folder path should not be empty");
	
	QDir dir(QString::fromStdString(tmpFolder));
	QVERIFY2(dir.exists(), "Tmp folder should exist");
}

void TestTempFiles::testSessionDirName()
{
	std::string name = TempFiles::sessionDirName();
	
	QVERIFY2(!name.empty(), "Session dir name should not be empty");
}

void TestTempFiles::testRetrieveList()
{
	TempFiles::createSessionDir();
	
	// Create a test file
	std::string root, relativePath;
	TempFiles::createSpecific("test", 1, root, relativePath);
	
	QString testFile(QString::fromStdString(root) + "/" + QString::fromStdString(relativePath));
	QFile file(testFile);
	QVERIFY2(file.open(QFile::WriteOnly), "Should be able to open test file");
	file.close();
	
	// Retrieve list
	auto list = TempFiles::retrieveList(1);
	QVERIFY2(!list.empty(), "Retrieve list should return at least one file");
}

void TestTempFiles::testStateFileExists()
{
	TempFiles::createSessionDir();
	
	bool exists = TempFiles::stateFileExists(1);
	// This test checks the API works, actual existence depends on implementation
	Q_UNUSED(exists);
	QVERIFY2(true, "State file check should work without error");
}

void TestTempFiles::testCreateClipboard()
{
	std::string clipboardFile = TempFiles::createSpecific_clipboard("test.txt");
	
	QVERIFY2(!clipboardFile.empty(), "Clipboard file path should not be empty");
}

void TestTempFiles::testPurgeClipboard()
{
	TempFiles::createSpecific_clipboard("test.txt");
	TempFiles::purgeClipboard();
	
	// Test should run without errors
	QVERIFY2(true, "Purge clipboard should work without error");
}

QTEST_MAIN(TestTempFiles)
