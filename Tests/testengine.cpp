#include "dirs.h"
#include "testengine.h"
#include <QSignalSpy>
#include "testinfo.h"
#include "tempfiles.h"
#include "processinfo.h"
#include "utilities/qutils.h"
#include "engine/enginesync.h"
#include "utilities/appdirs.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
#include "engine/enginerepresentation.h"

void TestEngine::initTestCase()
{
	Dirs::setLocalAppdataDir(AppDirs::appData(false).toStdString());
	TempFiles::init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory

}

void TestEngine::init()
{
	TempFiles::clearSessionDir();
	
	_pkg		= new DataSetPackage(this);
	_importer	= new CSVImporter();
	_engines	= new EngineSync(this);
	
	_engines->start();
	
	_engineRep	= _engines->createNewEngine(true, 0);
	
	
	_importer->loadDataSet(fq(_testLibrary().absoluteFilePath("csv/debug.csv")), [](int i){});

	_data		= _pkg->dataSet();
}

void TestEngine::cleanup()
{
	if(_engineRep)
		_engineRep->shutEngineDown();
	delete _engineRep;
	_engineRep = nullptr;
	
	delete _engines;
	_engines = nullptr;
	
	delete _data;
	_data = nullptr;
	
	DatabaseInterface::singleton()->close();
	DatabaseInterface::singleton()->closeInterfaces();
	
	delete _pkg;
	_pkg = nullptr;
	
	delete _importer;
	_importer = nullptr;
}

void TestEngine::testComputedColumns()
{
	QVERIFY2(_data,			"No dataset!");
	QVERIFY2(_engines,		"No EngineSync!");
	QVERIFY2(_engineRep,	"No EngineRepresentation!");
	
	_engines->startStoppedEngine(_engineRep);
	
	// const QString & columnName, const QString & warning, bool dataChanged
	QSignalSpy spy(_engineRep, SIGNAL(computeColumnSucceeded(const QString &, const QString &, bool))); 
	
	QVERIFY2(spy.isValid(),	"Spy is broken!");
		
	Column * col = _data->column("contBinom");
	col->setCodeType(computedColumnType::rCode);
	col->setRCode("V1");
	
	_engines->computeColumn("contBinom", tq(col->rCode()), columnType::ordinal);
	
	spy.wait(std::chrono::seconds(100));
	
	QVERIFY2(spy.count() == 1,	"Did not get a response");
	
	QVariantList response = spy.takeFirst();
	
	QVERIFY2(response[0].toString() == "contBinom",	"Did not get the right column back in response");
	QVERIFY2(response[1].toString() != "",			"Got a warning!");
	QVERIFY2(response[2].toBool(),					"Did not get dataChanged back in response");
	
	QVERIFY2(_data->column("contBinom")->jsonForCompare() == _data->column("V1")->jsonForCompare(), "Columns are not the same");
}


QTEST_MAIN(TestEngine)
