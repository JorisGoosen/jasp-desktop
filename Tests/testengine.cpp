#include "dirs.h"
#include "qutils.h"
#include "testengine.h"
#include <QSignalSpy>
#include "testinfo.h"
#include "tempfiles.h"
#include "processinfo.h"
#include "engine/enginesync.h"
#include "utilities/appdirs.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
#include "engine/enginerepresentation.h"
#include "filter.h"
#include "variableinfo.h"

void TestEngine::initTestCase()
{

}

void TestEngine::init()
{
	TempFiles	::	clearSessionDir();
	Dirs		::	setLocalAppdataDir(AppDirs::appData(false).toStdString());
	TempFiles	::	init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory

	_pkg		=	new DataSetPackage(this);
	_importer	=	new CSVImporter();
	_engines	=	new EngineSync(this);

	_engines	->	start();
	_engineRep	=	_engines->createNewEngine(true, 0);
	_importer	->	loadDataSet(fq(_testLibrary().absoluteFilePath("csv/debug.csv")), _pkg->createDataSet(), [](int i){});
	_data		=	_pkg->dataSet();
}

void TestEngine::cleanup()
{
	if(_engineRep)
		_engineRep->shutEngineDown();
	
	//Gets cleaned up by EngineSync
	_engineRep = nullptr;

	delete _engines;
	_engines = nullptr;

	DatabaseInterface::singleton()->close();
	DatabaseInterface::singleton()->closeInterfaces();

	delete _pkg;
	_pkg = nullptr;

	_data = nullptr;

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
	
	if(!_data->column("V1")->hasLabels())
		_data->column("V1")->noLabelsToLabels();	
		
	Column * col = _data->column("contBinom");
	col->setCodeType(computedColumnType::rCode);
	col->setRCode("V1");
	
	_engines->computeColumn(_data->id(), "contBinom", tq(col->rCode()), columnType::ordinal);
	
	spy.wait();
	
	QVERIFY2(spy.count() == 1,	"Did not get a response");
	
	QVariantList response = spy.takeFirst();
	spy.clear();
	
	QVERIFY2(response[0].toString() == "contBinom",	"Did not get the right column back in response");
	QVERIFY2(response[1].toString() == "",			"Got a warning!");
	QVERIFY2(response[2].toBool(),					"Did not get dataChanged back in response");

	col->checkForUpdates();
	

	Json::Value		jsonContBinom	= col->jsonForCompare(),
					jsonV1			= _data->column("V1")->jsonForCompare();

	std::cout << jsonContBinom.toStyledString() << "\n" << jsonV1.toStyledString() << std::endl;
	
	QVERIFY2(jsonContBinom["labels"] == jsonV1["labels"], "Labels are not the same");
	QVERIFY2(jsonContBinom["data"]   == jsonV1["data"],   "Data is not the same");



	//Now lets see if it can also not be the same:
	col->setRCode("V1+1");

	_engines->computeColumn(_data->id(), "contBinom", tq(col->rCode()), columnType::scale);

	spy.wait();

	QVERIFY2(spy.count() == 1,	"Did not get a response");

	response = spy.takeFirst();
	spy.clear();

	QVERIFY2(response[0].toString() == "contBinom",	"Did not get the right column back in response");
	QVERIFY2(response[1].toString() == "",			"Got a warning!");
	QVERIFY2(response[2].toBool(),					"Did not get dataChanged back in response");

	col->checkForUpdates();

	jsonContBinom	= _data->column("contBinom")->jsonForCompare();
	jsonV1			= _data->column("V1")->jsonForCompare();
	
	std::cerr << jsonContBinom["labels"].toStyledString() << "\n" << jsonV1["labels"].toStyledString() << std::endl;

	QVERIFY2(jsonContBinom["data"]   != jsonV1["data"],   "Data is the same, but they shouldnt be");

	Column * col2 = _data->column("contcor1");
	col2->setCodeType(computedColumnType::rCode);
	col2->setRCode("contBinom-1"); //Should make it the same as V1 again
	
	if(!col2->hasLabels())
		col2->noLabelsToLabels();

	_engines->computeColumn(_data->id(), "contcor1", tq(col2->rCode()), columnType::scale);

	spy.wait();

	QVERIFY2(spy.count() == 1,	"Did not get a response");

	response = spy.takeFirst();
	spy.clear();

	QVERIFY2(response[0].toString() == "contcor1",	"Did not get the right column back in response");
	QVERIFY2(response[1].toString() == "",			"Got a warning!");
	QVERIFY2(response[2].toBool(),					"Did not get dataChanged back in response");

	col2->checkForUpdates();

	Json::Value		jsonContCor1	= col2->jsonForCompare();

	//std::cout << jsonContCor1.toStyledString() /*<< "\n" << jsonV1.toStyledString()*/ << std::endl;

	QVERIFY2(jsonContCor1["data"]   == jsonV1["data"],   "Data is not the same");
	QVERIFY2(jsonContCor1["labels"] == jsonV1["labels"], "Labels are not the same");

}

void TestEngine::testFilters()
{
	QVERIFY2(_data,			"No dataset!");
	QVERIFY2(_engines,		"No EngineSync!");
	QVERIFY2(_engineRep,	"No EngineRepresentation!");

	_engines->startStoppedEngine(_engineRep);

	QSignalSpy spy(_engineRep, SIGNAL(filterDone(int)));

	QVERIFY2(spy.isValid(),	"Spy is broken!");

	_data->defaultFilter()->setRFilter("V1%%2==0");
	_engines->sendFilter(_data->id(), "", tq(_data->defaultFilter()->rFilter()));

	spy.wait();

	QVERIFY2(spy.count() == 1,	"Did not get a response");

	QVariantList response = spy.takeFirst();
	spy.clear();

	std::cout << "Response was: " << response[0].toString() << std::endl;

	_data->checkForUpdates();

	QVERIFY2(_data->defaultFilter()->filteredRowCount() == _data->rowCount() / 2,	"Did not get right filtered rowCount!");
	QVERIFY2(!_data->defaultFilter()->filtered()[0],								"Expected first filtered to be FALSE");
	QVERIFY2(_data->defaultFilter()->filtered()[1],									"Expected second filtered to be TRUE");

}

void TestEngine::testVariableInfoPerFilter()
{
	DataSet * ds = _data;

	// Programmatically ensure data exists (CSV import may be environment-dependent)
	if(ds->columnCount() == 0)
	{
		ds->beginBatchedToDB();
		ds->setColumnCount(2);
		ds->setRowCount(6);
		auto lookup = [](size_t r) -> std::string { return std::to_string(int(r + 1)); };
		ds->column(0)->initFromLookups("V1", 6, lookup, lookup, "V1", columnType::unknown, {}, 10, true);
		auto lookup2 = [](size_t r) -> std::string { return std::string(1, char('a' + r)); };
		ds->column(1)->initFromLookups("V2", 6, lookup2, lookup2, "V2", columnType::unknown, {}, 10, true);
		ds->endBatchedToDB([](float){});
	}

	QCOMPARE(ds->columnCount(),		2);
	QCOMPARE(ds->rowCount(),		6);

	int rowCount		 = ds->rowCount();
	int half			 = rowCount / 2;
	
	ds->defaultFilter()->setFilterVector(boolvec(true, rowCount));
	QCOMPARE(ds->defaultFilter()->filteredRowCount(), rowCount);

	Filter * filterEven = ds->createFilter("filterEven", true);
	Filter * filterOdd  = ds->createFilter("filterOdd",  true);

	QVERIFY2(filterEven != ds->defaultFilter(),	"Named filter should be different from default");
	QVERIFY2(filterOdd  != ds->defaultFilter(),	"Named filter should be different from default");
	QVERIFY2(filterEven != filterOdd,			"Two named filters should be different");

	boolvec evenFilter(rowCount, false);
	for(int i=0; i<rowCount; i+=2)	evenFilter[i] = true;
	QVERIFY2(filterEven->setFilterVector(evenFilter),	"Could not set filter vector for filterEven");

	boolvec oddFilter(rowCount, false);
	for(int i=1; i<rowCount; i+=2)	oddFilter[i] = true;
	QVERIFY2(filterOdd->setFilterVector(oddFilter),		"Could not set filter vector for filterOdd");

	QCOMPARE(filterEven->filteredRowCount(), half);
	QCOMPARE(filterOdd->filteredRowCount(),  half);

	VariableInfo info;
	info.setProvider(filterEven);

	QCOMPARE(info.rowCount(),			filterEven->filteredRowCount());
	QCOMPARE(info.variableCount(),		ds->columnCount());

	QStringList vars = info.provider()->provideInfo(varInfoType::VariableNames).toStringList();
	QVERIFY2(vars.size() > 0,			"Should have variable names");
	QVERIFY2(vars.contains("V1"),		"Should contain V1");
	QVERIFY2(vars.contains("V2"),		"Should contain V2");

	info.setProvider(filterOdd);
	QCOMPARE(info.rowCount(),			filterOdd->filteredRowCount());
	QCOMPARE(info.variableCount(),		ds->columnCount());

	QStringList varsOdd = info.provider()->provideInfo(varInfoType::VariableNames).toStringList();
	QCOMPARE(vars, varsOdd);

	Filter * defaultF = ds->defaultFilter();
	QCOMPARE(defaultF->filteredRowCount(), rowCount);

	info.setProvider(defaultF);
	QCOMPARE(info.rowCount(), rowCount);
}


QTEST_MAIN(TestEngine)
