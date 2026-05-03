#ifndef TESTTEMPFILES_H
#define TESTTEMPFILES_H

#include <QObject>
#include <QTest>

class TestTempFiles : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void init();
	void cleanup();
	void testInit();
	void testCreateSessionDir();
	void testClearSessionDir();
	void testCreateSpecific();
	void testCreateTmpFolder();
	void testSessionDirName();
	void testRetrieveList();
	void testStateFileExists();
	void testCreateClipboard();
	void testPurgeClipboard();
};

#endif // TESTTEMPFILES_H
