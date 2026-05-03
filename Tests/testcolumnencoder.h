#ifndef TESTCOLUMNENCODER_H
#define TESTCOLUMNENCODER_H

#include <QObject>
#include <QTest>

class TestColumnEncoder : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void testEncode();
	void testDecode();
	void testEncodeDecodeRoundTrip();
	void testShouldEncode();
	void testShouldDecode();
	void testSetCurrentNames();
	void testIsColumnName();
	void testIsEncodedColumnName();
	void testEncodeRScript();
	void testColumnNames();
	void testColumnNamesEncoded();
};

#endif // TESTCOLUMNENCODER_H
