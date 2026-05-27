#include "dirs.h"
#include "qutils.h"
#include "testengine.h"
#include <QSignalSpy>
#include "testinfo.h"
#include "tempfiles.h"
#include "processinfo.h"
#include "engine/enginesync.h"
#include "utilities/appdirs.h"
#include "utilities/settings.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
#include "engine/enginerepresentation.h"
#include "workspace.h"
#include "datasetsyncer.h"

void TestEngine::initTestCase()
{

}

void TestEngine::init()
{
	TempFiles	::	clearSessionDir();
	Dirs		::	setLocalAppdataDir(AppDirs::appData(false).toStdString());
	TempFiles	::	init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory
	Settings	::	informSettingsThatThisIsATest();
	
	_pkg		=	new DataSetPackage(this);
	_importer	=	new CSVImporter();
	_engines	=	new EngineSync(this);

	_engines	->	start();
	_engineRep	=	_engines->createNewEngine(true, 0);
	_pkg->createDataSet();
	_importer	->	loadDataSet(fq(_testLibrary().absoluteFilePath("csv/debug.csv")), _pkg->dataSet(), [](int i){});
	_data		=	_pkg->dataSet();
}

void TestEngine::cleanup()
{
	if(_engineRep)
		_engineRep->shutEngineDown();
	
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
	
	if(!_data->column(std::string("V1"))->hasLabels())
		_data->column(std::string("V1"))->noLabelsToLabels();	
		
	Column * col = _data->column(std::string("contBinom"));
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
					jsonV1			= _data->column(std::string("V1"))->jsonForCompare();

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

	jsonContBinom	= _data->column(std::string("contBinom"))->jsonForCompare();
	jsonV1			= _data->column(std::string("V1"))->jsonForCompare();
	
	std::cerr << jsonContBinom["labels"].toStyledString() << "\n" << jsonV1["labels"].toStyledString() << std::endl;

	QVERIFY2(jsonContBinom["data"]   != jsonV1["data"],   "Data is the same, but they shouldnt be");

	Column * col2 = _data->column(std::string("contcor1"));
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

void TestEngine::testComputedDataSets()
{
	QVERIFY2(_data,		"No dataset!");
	
	Workspace * ws = Workspace::singleton();
	QVERIFY2(ws,		"No workspace!");

	// 1. Create a computed dataset depending on the source dataset
	int sourceId = _data->id();
	DataSet * compDs = ws->createComputedDataSet(
		"Computed Test",
		"data$V1 + 1",
		std::to_string(sourceId)
	);
	QVERIFY2(compDs,												"Computed dataset not created!");
	QVERIFY2(compDs->id() > 0,										"Computed dataset has invalid ID!");
	QVERIFY2(compDs->isComputed(),									"isComputed not set!");
	QVERIFY2(compDs->computeCode() == "data$V1 + 1",				"computeCode not stored correctly!");
	QVERIFY2(compDs->dependsOn() == std::to_string(sourceId),		"dependsOn not stored correctly!");

	// 2. Verify it appears in workspace
	DataSet * found = ws->dataSetById(compDs->id());
	QVERIFY2(found == compDs,										"Computed dataset not found in workspace!");

	// 3. Verify it coexists with the source dataset
	DataSet * source = ws->dataSetById(sourceId);
	QVERIFY2(source == _data,										"Source dataset missing from workspace!");
	QVERIFY2(!source->isComputed(),									"Source dataset should not be computed!");

	// 4. Test dependency-based refresh triggers for matching source
	QSignalSpy startSpy(ws, &Workspace::synchingStart);
	ws->refreshComputedDataSets(sourceId);
	QVERIFY2(startSpy.count() >= 1,									"Refresh not triggered for matching dependency!");

	// 5. Test that unrelated source does NOT trigger refresh
	QSignalSpy unrelatedSpy(ws, &Workspace::synchingStart);
	ws->refreshComputedDataSets(99999); // non-existent ID
	QVERIFY2(unrelatedSpy.count() == 0,								"Unrelated source should not trigger refresh!");

	// 6. Test multiple computed datasets with different dependencies
	DataSet * compDs2 = ws->createComputedDataSet(
		"Computed Test 2",
		"data$V1 * 2",
		std::to_string(sourceId) + "," + std::to_string(compDs->id())
	);
	QVERIFY2(compDs2,												"Second computed dataset not created!");
	QVERIFY2(compDs2->isComputed(),									"Second computed: isComputed not set!");

	// Refreshing source should trigger both computed datasets
	QSignalSpy multiSpy(ws, &Workspace::synchingStart);
	ws->refreshComputedDataSets(sourceId);
	QVERIFY2(multiSpy.count() >= 2,									"Should trigger both computed datasets!");

	// Cleanup computed datasets
	ws->stopSyncForDataSet(compDs);
	ws->stopSyncForDataSet(compDs2);
}


QTEST_MAIN(TestEngine)
