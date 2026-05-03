#ifndef JSONUTILITIESTYPEST_H
#define JSONUTILITIESTYPEST_H

#include <QObject>
#include <QTest>

class JsonUtilitiesTests : public QObject
{
	Q_OBJECT

private slots:
	void testVecToJsonArrayInt();
	void testVecToJsonArrayDouble();
	void testVecToJsonArrayDoubleWithNanInf();
	void testVecToJsonArrayString();
	void testSetToJsonArray();
	void testJsonStringArrayToVec();
	void testJsonStringArrayToSet();
	void testRemoveColumnsFromDragNDropFilterJSON();
	void testReplaceColumnNamesInDragNDropFilterJSON();
	void testRemoveColumnsFromDragNDropFilterJSONStr();
	void testReplaceColumnNamesInDragNDropFilterJSONStr();
};

#endif // JSONUTILITIESTYPEST_H
