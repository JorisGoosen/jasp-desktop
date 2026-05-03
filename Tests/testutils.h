#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <QObject>
#include <QTest>

class TestUtils : public QObject
{
	Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testCurrentDateTime();
	void testCurrentMillis();
	void testCurrentSeconds();
	void testGetFileSize();
	void testRemoveFile();
	void testRenameOverwrite();
	void testTouch();
	void testGetTypeFromFileName();
	void testOsPath();
	void testRemove();
	void testIsEqualFloat();
	void testIsEqualDouble();
};

#endif // TESTUTILS_H
