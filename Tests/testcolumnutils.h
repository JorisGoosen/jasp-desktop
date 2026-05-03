#ifndef TESTCOLUMNUTILS_H
#define TESTCOLUMNUTILS_H

#include <QObject>
#include <QTest>

class TestColumnUtils : public QObject
{
	Q_OBJECT

private slots:
	void testGetIntValueFromString();
	void testGetIntValueFromDouble();
	void testGetDoubleValue();
	void testGetDoubleValues();
	void testIsIntValue();
	void testIsDoubleValue();
	void testDoubleToLocale();
	void testDoubleToString();
	void testDoubleToStringMaxPrec();
	void testCurrencyString();
	void testConvertVecToInt();
	void testConvertVecToDouble();
	void testDecimalPoint();
	void testConvertEscapedUnicodeToUTF8();
	void testDeEuropeaniseForImport();
};

#endif // TESTCOLUMNUTILS_H
