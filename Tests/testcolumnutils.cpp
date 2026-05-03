#include "testcolumnutils.h"
#include "columnutils.h"
#include <set>
#include <limits>

void TestColumnUtils::testGetIntValueFromString()
{
	int value;
	
	bool result = ColumnUtils::getIntValue("42", value);
	QVERIFY2(result, "Should parse integer string");
	QVERIFY2(value == 42, "Parsed value should be 42");
	
	result = ColumnUtils::getIntValue("-17", value);
	QVERIFY2(result, "Should parse negative integer");
	QVERIFY2(value == -17, "Parsed value should be -17");
	
	result = ColumnUtils::getIntValue("not_a_number", value);
	QVERIFY2(!result, "Should fail to parse non-numeric string");
}

void TestColumnUtils::testGetIntValueFromDouble()
{
	int value;
	
	bool result = ColumnUtils::getIntValue(42.0, value);
	QVERIFY2(result, "Should convert integer double to int");
	QVERIFY2(value == 42, "Should have correct value");
	
	result = ColumnUtils::getIntValue(-17.0, value);
	QVERIFY2(result, "Should convert negative integer double to int");
	QVERIFY2(value == -17, "Should have correct negative value");
	
	result = ColumnUtils::getIntValue(42.7, value);
	QVERIFY2(!result, "Should reject non-integer double");
}

void TestColumnUtils::testGetDoubleValue()
{
	double value;
	
	bool result = ColumnUtils::getDoubleValue("3.14", value);
	QVERIFY2(result, "Should parse decimal string");
	QVERIFY2(std::abs(value - 3.14) < 0.0001, "Parsed value should be close to 3.14");
	
	result = ColumnUtils::getDoubleValue("-2.718", value);
	QVERIFY2(result, "Should parse negative decimal");
	QVERIFY2(std::abs(value - (-2.718)) < 0.0001, "Parsed value should be close to -2.718");
	
	result = ColumnUtils::getDoubleValue("not_a_number", value);
	QVERIFY2(!result, "Should fail to parse non-numeric string");
}

void TestColumnUtils::testGetDoubleValues()
{
	std::set<std::string> values = {"1.5", "2.7", "3.14", "invalid", "4.0"};
	
	std::set<double> doubleValues = ColumnUtils::getDoubleValues(values, false);
	
	QVERIFY2(doubleValues.size() == 4, "Should have 4 valid doubles (excluding invalid)");
	
	bool found1_5 = false, found2_7 = false, found3_14 = false, found4_0 = false;
	for(double d : doubleValues) {
		if(std::abs(d - 1.5) < 0.0001) found1_5 = true;
		if(std::abs(d - 2.7) < 0.0001) found2_7 = true;
		if(std::abs(d - 3.14) < 0.0001) found3_14 = true;
		if(std::abs(d - 4.0) < 0.0001) found4_0 = true;
	}
	
	QVERIFY2(found1_5, "Should contain 1.5");
	QVERIFY2(found2_7, "Should contain 2.7");
	QVERIFY2(found3_14, "Should contain 3.14");
	QVERIFY2(found4_0, "Should contain 4.0");
}

void TestColumnUtils::testIsIntValue()
{
	QVERIFY2(ColumnUtils::isIntValue("42"), "Should recognize integer string");
	QVERIFY2(ColumnUtils::isIntValue("-17"), "Should recognize negative integer");
	QVERIFY2(!ColumnUtils::isIntValue("3.14"), "Should not recognize decimal as integer");
	QVERIFY2(!ColumnUtils::isIntValue("abc"), "Should not recognize letters as integer");
}

void TestColumnUtils::testIsDoubleValue()
{
	QVERIFY2(ColumnUtils::isDoubleValue("3.14"), "Should recognize decimal string");
	QVERIFY2(ColumnUtils::isDoubleValue("42"), "Should recognize integer as double");
	QVERIFY2(ColumnUtils::isDoubleValue("-2.718"), "Should recognize negative decimal");
	QVERIFY2(!ColumnUtils::isDoubleValue("abc"), "Should not recognize letters as double");
}

void TestColumnUtils::testDoubleToLocale()
{
	std::string result = ColumnUtils::doubleToLocale("3.14159");
	QVERIFY2(!result.empty(), "Result should not be empty");
}

void TestColumnUtils::testDoubleToString()
{
	std::string result = ColumnUtils::doubleToString(3.14159);
	QVERIFY2(!result.empty(), "Result should not be empty");
	
	result = ColumnUtils::doubleToString(42.0);
	QVERIFY2(!result.empty(), "Integer double should convert");
	
	result = ColumnUtils::doubleToString(-2.718, true, 2);
	QVERIFY2(!result.empty(), "Negative double with precision should convert");
}

void TestColumnUtils::testDoubleToStringMaxPrec()
{
	std::string result = ColumnUtils::doubleToStringMaxPrec(3.14159265359);
	QVERIFY2(!result.empty(), "Result should not be empty");
}

void TestColumnUtils::testCurrencyString()
{
	std::string result = ColumnUtils::currencyString(1234.56);
	QVERIFY2(!result.empty(), "Currency string should not be empty");
	
	result = ColumnUtils::currencyString(999.99, "$");
	QVERIFY2(!result.empty(), "Currency with symbol should not be empty");
}

void TestColumnUtils::testConvertVecToInt()
{
	std::vector<std::string> values = {"1", "2", "3", "4", "5"};
	std::vector<int> intValues;
	std::set<int> uniqueValues;
	
	bool result = ColumnUtils::convertVecToInt(values, intValues, uniqueValues);
	
	QVERIFY2(result, "Should convert successfully");
	QVERIFY2(intValues.size() == 5, "Should have 5 converted values");
	QVERIFY2(uniqueValues.size() == 5, "Should have 5 unique values");
	
	result = ColumnUtils::convertVecToInt({"1", "invalid", "3"}, intValues, uniqueValues);
	QVERIFY2(!result, "Should fail when encountering invalid value");
}

void TestColumnUtils::testConvertVecToDouble()
{
	std::vector<std::string> values = {"1.5", "2.7", "3.14", "4.0"};
	std::vector<double> doubleValues;
	
	bool result = ColumnUtils::convertVecToDouble(values, doubleValues);
	
	QVERIFY2(result, "Should convert successfully");
	QVERIFY2(doubleValues.size() == 4, "Should have 4 converted values");
	
	result = ColumnUtils::convertVecToDouble({"1.5", "invalid"}, doubleValues);
	QVERIFY2(!result, "Should fail when encountering invalid value");
}

void TestColumnUtils::testDecimalPoint()
{
	ColumnUtils::setDecimalPoint(".");
	QVERIFY2(ColumnUtils::decimalPoint() == ".", "Decimal point should be set to dot");
	
	ColumnUtils::setDecimalPoint(",");
	QVERIFY2(ColumnUtils::decimalPoint() == ",", "Decimal point should be set to comma");
}

void TestColumnUtils::testConvertEscapedUnicodeToUTF8()
{
	std::string input = "Hello\\u0020World";  // \u0020 is space
	ColumnUtils::convertEscapedUnicodeToUTF8(input);
	QVERIFY2(!input.empty(), "Result should not be empty");
}

void TestColumnUtils::testDeEuropeaniseForImport()
{
	std::string result = ColumnUtils::deEuropeaniseForImport("3,14159");
	QVERIFY2(result.find(".") != std::string::npos || result.find(",") != std::string::npos,
			 "Result should contain decimal separator");
	
	result = ColumnUtils::deEuropeaniseForImport("1.000,50");
	QVERIFY2(!result.empty(), "Result should not be empty");
}

QTEST_MAIN(TestColumnUtils)
