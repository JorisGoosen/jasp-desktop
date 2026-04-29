#include "testcsvpreviewmodel.h"
#include "utilities/csvpreviewmodel.h"


void TestCsvPreviewModel::testCsvParsing()
{
    CsvPreviewModel model;
    
    QString rawData = "Col1,Col2,Col3\nVal1,Val2,Val3\nVal4,Val5,Val6";
    model.preparePreview(rawData.toStdString().c_str(), ',');

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 3);

    // Check first row (header)
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QString("Col1"));
    QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QString("Col2"));
    QCOMPARE(model.data(model.index(0, 2), Qt::DisplayRole).toString(), QString("Col3"));

    // Check second row
    QCOMPARE(model.data(model.index(1, 0), Qt::DisplayRole).toString(), QString("Val1"));
    QCOMPARE(model.data(model.index(1, 1), Qt::DisplayRole).toString(), QString("Val2"));
    QCOMPARE(model.data(model.index(1, 2), Qt::DisplayRole).toString(), QString("Val3"));

    // Check third row
    QCOMPARE(model.data(model.index(2, 0), Qt::DisplayRole).toString(), QString("Val4"));
    QCOMPARE(model.data(model.index(2, 1), Qt::DisplayRole).toString(), QString("Val5"));
    QCOMPARE(model.data(model.index(2, 2), Qt::DisplayRole).toString(), QString("Val6"));
}


QTEST_MAIN(TestCsvPreviewModel)
