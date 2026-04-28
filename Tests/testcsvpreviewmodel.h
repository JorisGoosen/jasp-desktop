#include <QtQuickTest>
#include <QQmlEngine>
#include <CsvPreviewModel>
#include <QDebug>
#include <QAbstractItemModel>

class TestCsvPreviewModel : public QObject
{
    Q_OBJECT
private slots:
    void testCsvParsing();
};

QUICK_TEST_MAIN_WITH_SETUP(testcsvpreviewmodel, TestCsvPreviewModel)

#include "testcsvpreviewmodel.moc"
