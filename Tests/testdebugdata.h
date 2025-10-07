#ifndef TESTDEBUGDATA_H
#define TESTDEBUGDATA_H

#include <QTest>

class DataSetPackage;
class Importer;
class DataSet;

class TestDebugData: public QObject
{
    Q_OBJECT
private slots:
    void    initTestCase();
    void    init();
	void	cleanup();
    void    testReverseNumericals();
	void    testReverseLabels();
	

private:
	DataSetPackage		*	_pkg		= nullptr;
	Importer			*	_importer	= nullptr;
	DataSet				*	_data		= nullptr;
	const char			*	_debugCsv	= "";
};


#endif // TESTDEBUGDATA_H
