#include <QTest>

class DataSetPackage;
class Importer;

class TestAll: public QObject
{
    Q_OBJECT
private slots:
    void    initTestCase();
    void    init();
	void	cleanup();
    void    testDataImport();

private:
	DataSetPackage     *	_pkg = nullptr;
	Importer			*	_importer =  nullptr;
};
