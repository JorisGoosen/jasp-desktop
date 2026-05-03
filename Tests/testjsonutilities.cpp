#include "testjsonutilities.h"
#include "jsonutilities.h"
#include "utils.h"
#include <QSet>
#include <cmath>

void JsonUtilitiesTests::testVecToJsonArrayInt()
{
	std::vector<int> vec = {1, 2, 3, 4, 5};
	Json::Value result = JsonUtilities::vecToJsonArray(vec);
	
	QVERIFY2(result.isArray(), "Result should be an array");
	QVERIFY2(result.size() == 5, "Array should have 5 elements");
	
	for(int i = 0; i < (int)vec.size(); ++i)
		QVERIFY2(result[i].asInt() == vec[i], "Element mismatch");
}

void JsonUtilitiesTests::testVecToJsonArrayDouble()
{
	std::vector<double> vec = {1.5, 2.7, 3.14, 4.0, 5.99};
	Json::Value result = JsonUtilities::vecToJsonArray(vec);
	
	QVERIFY2(result.isArray(), "Result should be an array");
	QVERIFY2(result.size() == 5, "Array should have 5 elements");
	
	for(int i = 0; i < (int)vec.size(); ++i)
		QVERIFY2(std::abs(result[i].asDouble() - vec[i]) < 0.0001, "Element mismatch");
}

void JsonUtilitiesTests::testVecToJsonArrayDoubleWithNanInf()
{
	std::vector<double> vec = {1.5, std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), 5.0};
	Json::Value result = JsonUtilities::vecToJsonArray(vec);
	
	QVERIFY2(result.isArray(), "Result should be an array");
	QVERIFY2(result.size() == 5, "Array should have 5 elements");
	
	QVERIFY2(result[0].isDouble(), "First element should be a double");
	QVERIFY2(result[1].isNull(), "Infinity should be converted to null");
	QVERIFY2(result[2].isNull(), "Negative infinity should be converted to null");
	QVERIFY2(result[3].isNull(), "NaN should be converted to null");
	QVERIFY2(result[4].isDouble(), "Last element should be a double");
}

void JsonUtilitiesTests::testVecToJsonArrayString()
{
	std::vector<std::string> vec = {"hello", "world", "test", "json"};
	Json::Value result = JsonUtilities::vecToJsonArray(vec);
	
	QVERIFY2(result.isArray(), "Result should be an array");
	QVERIFY2(result.size() == 4, "Array should have 4 elements");
	
	for(int i = 0; i < (int)vec.size(); ++i)
		QVERIFY2(result[i].asString() == vec[i], "Element mismatch");
}

void JsonUtilitiesTests::testSetToJsonArray()
{
	std::set<int> set = {1, 2, 3, 4, 5};
	Json::Value result = JsonUtilities::setToJsonArray(set);
	
	QVERIFY2(result.isArray(), "Result should be an array");
	QVERIFY2(result.size() == 5, "Array should have 5 elements");
}

void JsonUtilitiesTests::testJsonStringArrayToVec()
{
	Json::Value json = Json::arrayValue;
	json.append("first");
	json.append("second");
	json.append("third");
	
	std::vector<std::string> result = JsonUtilities::jsonStringArrayToVec(json);
	
	QVERIFY2(result.size() == 3, "Vector should have 3 elements");
	QVERIFY2(result[0] == "first", "First element mismatch");
	QVERIFY2(result[1] == "second", "Second element mismatch");
	QVERIFY2(result[2] == "third", "Third element mismatch");
}

void JsonUtilitiesTests::testJsonStringArrayToSet()
{
	Json::Value json = Json::arrayValue;
	json.append("apple");
	json.append("banana");
	json.append("cherry");
	
	std::set<std::string> result = JsonUtilities::jsonStringArrayToSet(json);
	
	QVERIFY2(result.size() == 3, "Set should have 3 elements");
	QVERIFY2(result.count("apple"), "Set should contain apple");
	QVERIFY2(result.count("banana"), "Set should contain banana");
	QVERIFY2(result.count("cherry"), "Set should contain cherry");
}

void JsonUtilitiesTests::testRemoveColumnsFromDragNDropFilterJSON()
{
	Json::Value json;
	json["rowFilter"]["include"] = Json::arrayValue;
	json["rowFilter"]["include"].append("col1");
	json["rowFilter"]["include"].append("col2");
	json["rowFilter"]["include"].append("col3");
	
	Json::Value result = JsonUtilities::removeColumnsFromDragNDropFilterJSON(json, stringvec({"col2"}));
	
	QVERIFY2(result.isObject(), "Result should be an object");
}

void JsonUtilitiesTests::testRemoveColumnsFromDragNDropFilterJSONStr()
{
	std::string jsonStr = "{\"rowFilter\":{\"include\":[\"col1\",\"col2\",\"col3\"]}}";
	std::string result = JsonUtilities::removeColumnsFromDragNDropFilterJSONStr(jsonStr, stringvec({"col2"}));
	
	QVERIFY2(!result.empty(), "Result should not be empty");
}

void JsonUtilitiesTests::testReplaceColumnNamesInDragNDropFilterJSON()
{
	Json::Value json;
	json["rowFilter"]["include"] = Json::arrayValue;
	json["rowFilter"]["include"].append("oldName1");
	json["rowFilter"]["include"].append("oldName2");
	
	std::map<std::string, std::string> replacements;
	replacements["oldName1"] = "newName1";
	replacements["oldName2"] = "newName2";
	
	Json::Value result = JsonUtilities::replaceColumnNamesInDragNDropFilterJSON(json, replacements);
	
	QVERIFY2(result.isObject(), "Result should be an object");
}

void JsonUtilitiesTests::testReplaceColumnNamesInDragNDropFilterJSONStr()
{
	std::string jsonStr = "{\"rowFilter\":{\"include\":[\"oldName1\",\"oldName2\"]}}";
	
	std::map<std::string, std::string> replacements;
	replacements["oldName1"] = "newName1";
	replacements["oldName2"] = "newName2";
	
	std::string result = JsonUtilities::replaceColumnNamesInDragNDropFilterJSONStr(jsonStr, replacements);
	
	QVERIFY2(!result.empty(), "Result should not be empty");
}

QTEST_MAIN(JsonUtilitiesTests)
