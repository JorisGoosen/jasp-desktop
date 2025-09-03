#include <QTest>

class DataSetPackage;

class TestImport: public QObject
{
    Q_OBJECT
private slots:
    void    initTestCase();
    void    init();
    void    testDebugCsv();
    void    testDebugReadStat();

private:
    DataSetPackage     * _pkg = nullptr;
};
