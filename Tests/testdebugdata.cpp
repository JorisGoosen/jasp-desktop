#include "testinfo.h"
#include "tempfiles.h"
#include "processinfo.h"
#include "testdebugdata.h"
#include "utilities/qutils.h"
#include "databaseinterface.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
#include "data/importers/odsimporter.h"
#include "data/importers/excelimporter.h"
#include "data/importers/rdataimporter.h"
#include "data/importers/readstatimporter.h"

void TestDebugData::initTestCase()
{
	TempFiles::init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory

}

void TestDebugData::init()
{
	TempFiles::clearSessionDir();
	
	_pkg		= new DataSetPackage(this);
	_importer	= new CSVImporter();
	
	_importer->loadDataSet(fq(_testLibrary().absoluteFilePath("csv/debug.csv")), [](int i){});

	_data = _pkg->dataSet();
}

void TestDebugData::cleanup()
{
	if(_data)
		delete _data;
	_data = nullptr;
	
	DatabaseInterface::singleton()->close();
	DatabaseInterface::singleton()->closeInterfaces();
	
	if(_pkg)
		delete _pkg;
	_pkg = nullptr;
	
	if(_importer)
		delete _importer;
	_importer = nullptr;
}


void TestDebugData::testReverseNumericals()
{
	QVERIFY2(_data,						"No dataset!");
}


QTEST_MAIN(TestDebugData)
