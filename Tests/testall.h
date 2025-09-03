#include <QTest>

class DataSetPackage;

class TestAll: public QObject
{
    Q_OBJECT
private slots:
    void    initTestCase();
    void    init();
    void    testDataImport();

private:
    DataSetPackage     * _pkg = nullptr;
};
