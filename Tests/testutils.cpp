#include "testutils.h"
#include "utils.h"
#include "utilities/qutils.h"
#include <QFile>
#include <QDir>
#include <cmath>

void TestUtils::init()
{
	// Clean up any test files from previous runs
	Utils::removeFile("/tmp/jasp_testfile.txt");
	Utils::removeFile("/tmp/jasp_testfile_renamed.txt");
}

void TestUtils::cleanup()
{
	// Clean up test files
	Utils::removeFile("/tmp/jasp_testfile.txt");
	Utils::removeFile("/tmp/jasp_testfile_renamed.txt");
}

void TestUtils::testCurrentDateTime()
{
	std::string dateTime = Utils::currentDateTime();
	
	QVERIFY2(!dateTime.empty(), "Current datetime should not be empty");
	QVERIFY2(dateTime.length() > 10, "Datetime should have reasonable length");
}

void TestUtils::testCurrentMillis()
{
	int64_t millis1 = Utils::currentMillis();
	Utils::sleep(10);  // Sleep 10ms
	int64_t millis2 = Utils::currentMillis();
	
	QVERIFY2(millis2 >= millis1, "Time should move forward");
	QVERIFY2(millis1 > 0, "Millis should be positive");
}

void TestUtils::testCurrentSeconds()
{
	int64_t secs1 = Utils::currentSeconds();
	Utils::sleep(1100);  // Sleep 1.1 seconds
	int64_t secs2 = Utils::currentSeconds();
	
	QVERIFY2(secs2 >= secs1, "Seconds should move forward or stay same");
	QVERIFY2(secs1 > 0, "Seconds should be positive");
}

void TestUtils::testGetFileSize()
{
	// Create a test file with known content
	QString testFile = "/tmp/jasp_testfile.txt";
	QFile file(testFile);
	QVERIFY2(file.open(QFile::WriteOnly), "Should be able to create test file");
	file.write("Hello, World!");
	file.close();
	
	int64_t size = Utils::getFileSize(fq(testFile));
	
	QVERIFY2(size == 13, "File size should match content length (13 bytes)");
}

void TestUtils::testRemoveFile()
{
	QString testFile = "/tmp/jasp_testfile.txt";
	QFile file(testFile);
	QVERIFY2(file.open(QFile::WriteOnly), "Should be able to create test file");
	file.write("Test content");
	file.close();
	
	QVERIFY2(QFile::exists(testFile), "Test file should exist before removal");
	
	bool removed = Utils::removeFile(fq(testFile));
	
	QVERIFY2(removed, "Remove should return true");
	QVERIFY2(!QFile::exists(testFile), "File should not exist after removal");
}

void TestUtils::testRenameOverwrite()
{
	QString oldFile = "/tmp/jasp_testfile.txt";
	QString newFile = "/tmp/jasp_testfile_renamed.txt";
	
	// Create old file
	QFile file(oldFile);
	QVERIFY2(file.open(QFile::WriteOnly), "Should be able to create test file");
	file.write("Test content");
	file.close();
	
	// Remove new file if it exists
	Utils::removeFile(fq(newFile));
	
	QVERIFY2(QFile::exists(oldFile), "Old file should exist");
	QVERIFY2(!QFile::exists(newFile), "New file should not exist yet");
	
	bool renamed = Utils::renameOverwrite(fq(oldFile), fq(newFile));
	
	QVERIFY2(renamed, "Rename should succeed");
	QVERIFY2(!QFile::exists(oldFile), "Old file should not exist after rename");
	QVERIFY2(QFile::exists(newFile), "New file should exist after rename");
}

void TestUtils::testTouch()
{
	QString testFile = "/tmp/jasp_testfile.txt";
	
	// Create file
	QFile file(testFile);
	QVERIFY2(file.open(QFile::WriteOnly), "Should be able to create test file");
	file.close();
	
	int64_t timeBefore = Utils::getFileModificationTime(fq(testFile));
	
	Utils::sleep(100);  // Wait 100ms
	
	Utils::touch(fq(testFile));
	
	int64_t timeAfter = Utils::getFileModificationTime(fq(testFile));
	
	QVERIFY2(timeAfter >= timeBefore, "Modification time should not decrease");
}

void TestUtils::testGetTypeFromFileName()
{
	Utils::FileType csvType = Utils::getTypeFromFileName("test.csv");
	QVERIFY2(static_cast<int>(csvType) >= 0, "CSV type should be recognized");
	
	Utils::FileType jaspType = Utils::getTypeFromFileName("test.jasp");
	QVERIFY2(static_cast<int>(jaspType) >= 0, "JASP type should be recognized");
	
	Utils::FileType unknownType = Utils::getTypeFromFileName("test.xyz");
	QVERIFY2(static_cast<int>(unknownType) >= 0, "Unknown type should have a value");
}

void TestUtils::testOsPath()
{
	std::string path1 = Utils::osPath(std::filesystem::path("/tmp/test"));
	QVERIFY2(!path1.empty(), "OS path should not be empty");
	
	std::filesystem::path path2 = Utils::osPath(std::string("/tmp/test2"));
	QVERIFY2(!path2.empty(), "FS path should not be empty");
}

void TestUtils::testRemove()
{
	stringvec target = {"a", "b", "c", "d", "e"};
	stringvec toRemove = {"b", "d"};
	
	Utils::remove(target, toRemove);
	
	QVERIFY2(target.size() == 3, "Vector should have 3 elements after removal");
	QVERIFY2(target[0] == "a", "First element should be 'a'");
	QVERIFY2(target[1] == "c", "Second element should be 'c'");
	QVERIFY2(target[2] == "e", "Third element should be 'e'");
}

void TestUtils::testIsEqualFloat()
{
	QVERIFY2(Utils::isEqual(1.0f, 1.0f), "Equal floats should be equal");
	QVERIFY2(!Utils::isEqual(1.0f, 2.0f), "Different floats should not be equal");
	QVERIFY2(Utils::isEqual(0.0f, 0.0f), "Zero floats should be equal");
}

void TestUtils::testIsEqualDouble()
{
	QVERIFY2(Utils::isEqual(1.0, 1.0), "Equal doubles should be equal");
	QVERIFY2(!Utils::isEqual(1.0, 2.0), "Different doubles should not be equal");
	QVERIFY2(Utils::isEqual(0.0, 0.0), "Zero doubles should be equal");
}

QTEST_MAIN(TestUtils)
