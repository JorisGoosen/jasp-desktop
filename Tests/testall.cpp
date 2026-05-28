#include "testall.h"
#include "testinfo.h"
#include "tempfiles.h"
#include "processinfo.h"
#include "qutils.h"
#include "databaseinterface.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
#include "data/importers/odsimporter.h"
#include "data/importers/jaspimporter.h"
#include "data/exporters/jaspexporter.h"
#include "data/importers/excelimporter.h"
#include "data/importers/rdataimporter.h"
#include "data/importers/readstatimporter.h"
#include "utilities/settings.h"
#include "datasetsyncer.h"
#include "dataset.h"
#include "workspace.h"

#include <QSignalSpy>
#include <QFile>
#include <QFileInfo>


void TestAll::initTestCase()
{
	TempFiles::init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory
}

void TestAll::init()
{
	Settings::informSettingsThatThisIsATest();
	//_pkg->reset(false);
}

void TestAll::cleanup()
{
	
	delete _importer;
	_importer = nullptr;

	DatabaseInterface::singleton()->close();
	DatabaseInterface::singleton()->closeInterfaces();
	delete _pkg;
	_pkg = nullptr;
}

#define TO_STR2(x) #x
#define TO_STR(x) TO_STR2(x)


void TestAll::testDataImport_data()
{
	QTest::addColumn<QString>("folder");
	QTest::addColumn<QString>("dataFileAbsolutePath");

	for(const QString & folder : _testLibrary().entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
	{
		if(folder == "jasp")
			continue;

		QDir subDir(_testLibrary());
		subDir.cd(folder);

		for(QFileInfo & i : subDir.entryInfoList(QDir::Filter::Files | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
			if(i.suffix() != "json")
				QTest::newRow(i.fileName().toUtf8()) << folder << i.absoluteFilePath();
	}
}

void TestAll::testDataImport()
{
	QFETCH(QString, folder);
	QFETCH(QString, dataFileAbsolutePath);

	QDir subDir(_testLibrary());
	subDir.cd(folder);

	auto getImporter = [&]() -> Importer *
	{
		if(folder == "readstat")	return new ReadStatImporter();
		if(folder == "rdata")		return new RDataImporter();
		if(folder == "excel")		return new ExcelImporter();
		if(folder == "ods")			return new ods::ODSImporter();
		if(folder == "csv")			return new CSVImporter();

		return nullptr;
	};

	if(_pkg)
		delete _pkg;

	if(_importer)
		delete _importer;

	_pkg = new DataSetPackage(this);
	_importer = getImporter();

	QVERIFY2(_importer, "Getting importer failed...");

	std::cerr << "Testing " << dataFileAbsolutePath << std::endl;
	_importer->loadDataSet(fq(dataFileAbsolutePath), _pkg->createDataSet(), [](int i){});

	DataSet * dataSet = _pkg->dataSet();
	QVERIFY2(dataSet,						"No dataset!");

	Json::Value compareMe = dataSet->jsonForCompare();

	QString jsonFilePath = dataFileAbsolutePath,
			ext			 = QFileInfo(dataFileAbsolutePath).suffix();

	jsonFilePath.replace(jsonFilePath.size() - (ext.size() + 1), ext.size() + 1, ".json");

	QFileInfo jsonFileIn(jsonFilePath);

	if(!jsonFileIn.exists())
	{
		std::cerr << "Json does not exist yet, creating it now!" << std::endl;
		QFile jsonFile(jsonFilePath);
		jsonFile.open(QFile::OpenModeFlag::WriteOnly);
		jsonFile.write(compareMe.toStyledString().c_str());
		jsonFile.close();
	}

	QVERIFY(jsonFileIn.exists());

	QFile jsonFile(jsonFilePath);

	jsonFile.open(QFile::OpenModeFlag::ReadOnly);

	std::string jsonTxt  = fq(jsonFile.readAll());

	Json::Reader parser;
	Json::Value  hardcoded;

	QVERIFY2(parser.parse(jsonTxt, hardcoded),	"Parsing json failed!");

	bool hardcodedIsSame = hardcoded == compareMe;

	if(!hardcodedIsSame)
		std::cerr << stringUtils::replaceBy(compareMe.toStyledString(), "\n", " ") << std::endl;

	QVERIFY2(hardcodedIsSame,			"Hardcoded json is different!");

	
	DataSet loadMe(nullptr, dataSet->id());
	QVERIFY2(dataSet->jsonForCompare() == loadMe.jsonForCompare(), "DataSet isnt the same after dbload!");
}


void TestAll::testJaspDataImport_data()
{
	QTest::addColumn<QString>("folder");
	QTest::addColumn<QString>("dataFileAbsolutePath");

	for(const QString & folder : _testLibrary().entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
	{
		if(folder != "jasp")
			continue;

		QDir subDir(_testLibrary());
		subDir.cd(folder);

		for(QFileInfo & i : subDir.entryInfoList(QDir::Filter::Files | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
			if(i.suffix() != "json")
				QTest::newRow(i.fileName().toUtf8()) << folder << i.absoluteFilePath();
	}
}

void TestAll::testJaspRoundRobin_data()
{
	testJaspDataImport_data();
}

void TestAll::testJaspRoundRobin()
{
	QFETCH(QString, folder);
	QFETCH(QString, dataFileAbsolutePath);

	QDir subDir(_testLibrary());
	subDir.cd(folder);

	if(_pkg)
		delete _pkg;

	if(_importer)
		delete _importer;

	_pkg = new DataSetPackage(this);
	
	std::cerr << "Testing " << dataFileAbsolutePath << std::endl;
	JASPImporter::loadDataSet(fq(dataFileAbsolutePath),		[](int){});
	
	DataSet *	dataSet		= _pkg->dataSet();
	QVERIFY2(dataSet,			"No dataset!");
	
	Json::Value compareMe	= dataSet->jsonForCompare();
	std::string jaspFile	= TempFiles::createSpecific("testjasp", "temp.jasp");

	std::cerr << "Storing jasp file temporarily to: " << jaspFile << std::endl;
	JASPExporter().saveDataSet(jaspFile, [](int){});
	
	_pkg->reset();
	QVERIFY2(_pkg->dataSet()->jsonForCompare() != compareMe, "DataSet should be different after resetting DataSetPackage!");
	
	JASPImporter::loadDataSet(jaspFile, [](int){});
	
	dataSet = _pkg->dataSet();
	QVERIFY2(dataSet,									"No dataset!");
	QVERIFY2(dataSet->jsonForCompare() == compareMe,	"DataSet should be the same after reloading!");
}


void TestAll::testJaspDataImport()
{
	QFETCH(QString, folder);
	QFETCH(QString, dataFileAbsolutePath);

	QDir subDir(_testLibrary());
	subDir.cd(folder);

	if(_pkg)
		delete _pkg;

	if(_importer)
		delete _importer;

	_pkg = new DataSetPackage(this);
	
	std::cerr << "Testing " << dataFileAbsolutePath << std::endl;

	JASPImporter::loadDataSet(fq(dataFileAbsolutePath),		[](int){});
	
	DataSet * dataSet = _pkg->dataSet();
	QVERIFY2(dataSet,						"No dataset!");

	Json::Value compareMe = dataSet->jsonForCompare();

	QString jsonFilePath = dataFileAbsolutePath,
			ext			 = QFileInfo(dataFileAbsolutePath).suffix();

	jsonFilePath.replace(jsonFilePath.size() - (ext.size() + 1), ext.size() + 1, ".json");

	QFileInfo jsonFileIn(jsonFilePath);

	if(!jsonFileIn.exists())
	{
		std::cerr << "Json does not exist yet, creating it now!" << std::endl;
		QFile jsonFile(jsonFilePath);
		jsonFile.open(QFile::OpenModeFlag::WriteOnly);
		jsonFile.write(compareMe.toStyledString().c_str());
		jsonFile.close();

	}

	QVERIFY(jsonFileIn.exists());

	QFile jsonFile(jsonFilePath);
	
	
	jsonFile.open(QFile::OpenModeFlag::ReadOnly);

	std::string jsonTxt  = fq(jsonFile.readAll());

	Json::Reader parser;
	Json::Value  hardcoded;

	QVERIFY2(parser.parse(jsonTxt, hardcoded),	"Parsing json failed!");

	bool hardcodedIsSame = hardcoded == compareMe;

	if(!hardcodedIsSame)
		std::cerr << stringUtils::replaceBy(compareMe.toStyledString(), "\n", " ") << std::endl;

	QVERIFY2(hardcodedIsSame,			"Hardcoded json is different!");

	
	DataSet loadMe(nullptr, dataSet->id());
	QVERIFY2(dataSet->jsonForCompare() == loadMe.jsonForCompare(), "DataSet isnt the same after dbload!");
}

// Regression test for https://github.com/jasp-stats/jasp-desktop/commit/0a90b9a34e9d754f55bc32ec1efd2f67940ef756
// setDataSetSize() pre-allocates rows before initFromLookups() is called, causing rowCount() > 0
// when setValues() checks allTheSame — which skipped the label-detection loop and silently dropped
// all SPSS value labels.
void TestAll::testSavLabels()
{
	if(_pkg)	delete _pkg;
	if(_importer)	delete _importer;

	_pkg		= new DataSetPackage(this);
	_importer	= new ReadStatImporter();

	const QString savPath = _testLibrary().absoluteFilePath("readstat/Labelled_data.sav");
	_importer->loadDataSet(fq(savPath), _pkg->createDataSet(), [](int){});

	DataSet * dataSet = _pkg->dataSet();
	QVERIFY2(dataSet, "No dataset!");

	// These columns have SPSS value labels (e.g. 1->"Soha", 2->"Havonta vagy kevesebbszer", …)
	// and must be imported as labelled (nominal/ordinal) columns.
	const QStringList labelledColumns = {
		"AUDIT_gyakorisag",
		"AUDIT_mennyiség",
		"PHQ14_fejfajas",
		"PHQ14_szivveres",
		"PHQ9_energia"
	};

	for(const QString & colName : labelledColumns)
	{
		Column * col = dataSet->column(fq(colName));
		QVERIFY2(col,				qPrintable("Column not found: "	+ colName));
		QVERIFY2(col->hasLabels(),			qPrintable("Column has no labels: "  + colName));
		QVERIFY2(col->labels().size() > 0,	qPrintable("Label list is empty: "   + colName));
	}

	// Spot-check: AUDIT_gyakorisag label 1 should be "Soha"
	Column * audit = dataSet->column("AUDIT_gyakorisag");
	QVERIFY2(audit, "AUDIT_gyakorisag column not found");

	bool foundSoha = false;
	for(const Label * label : audit->labels())
		if(label->labelDisplay() == "Soha") { foundSoha = true; break; }

	QVERIFY2(foundSoha, "Expected label 'Soha' not found in AUDIT_gyakorisag");

	// Scale columns must NOT have labels
	const QStringList scaleColumns = { "Eletkor", "MHC_SF_Emo", "PSS_10" };
	for(const QString & colName : scaleColumns)
	{
		Column * col = dataSet->column(fq(colName));
		QVERIFY2(col, qPrintable("Column not found: " + colName));
		QVERIFY2(!col->hasLabels(), qPrintable("Scale column should not have labels: " + colName));
	}
}

// Regression test for https://github.com/jasp-stats/jasp-issues/issues/4293
void TestAll::testFilterLabels()
{
	if(_pkg)	delete _pkg;
	if(_importer)	delete _importer;

	_pkg		= new DataSetPackage(this);
	_importer	= new ReadStatImporter();

	const QString filePath = _testLibrary().absoluteFilePath("jasp/Directed Reading Activities.jasp");
	JASPImporter::loadDataSet(fq(filePath),		[](int){});

	DataSet * dataSet = _pkg->dataSet();
	QVERIFY2(dataSet, "No dataset!");

	std::string colName = "group";
	Column * col = dataSet->column(colName);
	QVERIFY2(col,										qPrintable("Group Column not found"));
	QVERIFY2(col->hasLabels(),							qPrintable("Group has no labels"));
	QVERIFY2(col->labelsNonEmptyCount() == 2,			qPrintable(tq("Number of labels is not 2: ")) + col->labelsNonEmptyCount());

	Label * controlLabel = col->labelByIndexNonEmpty(0);
	Label * treatLabel = col->labelByIndexNonEmpty(1);
	QVERIFY2(controlLabel->label() == "Control",		qPrintable("First label is not 'Control'"));
	QVERIFY2(controlLabel->filterAllows(),				qPrintable("'Control' label is filtered"));
	QVERIFY2(treatLabel->label() == "Treat",			qPrintable("Second label is not 'Treat'"));
	QVERIFY2(treatLabel->filterAllows(),				qPrintable("'Treat'label is filtered"));

	// Do as if the user clicked on Filter for the Control label in the Label window
	col->setLabelAllowFilter(0, false);
	QVERIFY2(!controlLabel->filterAllows(),				qPrintable("'Control' label is not filtered"));
	QVERIFY2(treatLabel->filterAllows(),				qPrintable("'Treat'label is filtered"));

	// Not all labels can be unset: nothing should change
	col->setLabelAllowFilter(1, false);
	QVERIFY2(!controlLabel->filterAllows(),				qPrintable("'Control' label is not filtered"));
	QVERIFY2(treatLabel->filterAllows(),				qPrintable("'Treat'label is filtered"));

	// Set first the Control label, and unset the Treat label: this time it should work
	col->setLabelAllowFilter(0, true);
	col->setLabelAllowFilter(1, false);
	QVERIFY2(controlLabel->filterAllows(),				qPrintable("'Control' label is filtered"));
	QVERIFY2(!treatLabel->filterAllows(),				qPrintable("'Treat'label is not filtered"));
}

// ---------- DataSetSyncer tests ----------

void TestAll::testSyncerStartStopFileSyncing()
{
	DataSet * ds = _pkg->dataSet();
	QVERIFY(ds);

	DataSetSyncer & syncer = ds->syncer();
	QVERIFY(!syncer.isFileSyncing());

	QTemporaryDir tempDir;
	QVERIFY(tempDir.isValid());
	QString testFilePath = tempDir.filePath("test_startstop.csv");
	QFile f(testFilePath);
	QVERIFY(f.open(QIODevice::WriteOnly));
	f.write("a,b,c\n1,2,3\n");
	f.close();

	syncer.startFileSyncing(testFilePath);
	QVERIFY(syncer.isFileSyncing());
	QCOMPARE(QString::fromStdString(ds->dataFilePath()), testFilePath);
	QVERIFY(ds->dataFileSynch());

	syncer.stopFileSyncing();
	QVERIFY(!syncer.isFileSyncing());
	QVERIFY(!ds->dataFileSynch());
}

void TestAll::testSyncerFileChangeEmitsSignal()
{
	DataSet * ds = _pkg->dataSet();
	QVERIFY(ds);

	DataSetSyncer & syncer = ds->syncer();

	QTemporaryDir tempDir;
	QVERIFY(tempDir.isValid());
	QString testFilePath = tempDir.filePath("sync_emit.csv");
	QFile f(testFilePath);
	QVERIFY(f.open(QIODevice::WriteOnly));
	f.write("x,y\n1,2\n");
	f.close();

	ds->setDataFileAndTimeStamp(testFilePath.toStdString(), 0);

	syncer.startFileSyncing(testFilePath);
	QVERIFY(syncer.isFileSyncing());

	QTest::qSleep(1200);

	QVERIFY(f.open(QIODevice::WriteOnly));
	f.write("x,y\n3,4\n");
	f.close();

	QTRY_COMPARE_WITH_TIMEOUT(syncer.isFileSyncing() ? 1 : 0, 1, 5000);

	// The file watcher signal is async; we check via syncRequired spy
	QSignalSpy spy(&syncer, &DataSetSyncer::syncRequired);
	QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);

	QList<QVariant> args = spy.takeFirst();
	QCOMPARE(args[0].toInt(), ds->id());
	QCOMPARE(args[1].toString(), testFilePath);
	QVERIFY(args[3].toString().isEmpty());

	syncer.stopFileSyncing();
}

void TestAll::testSyncerStartStopDatabaseSyncing()
{
	DataSet * ds = _pkg->dataSet();
	QVERIFY(ds);

	DataSetSyncer & syncer = ds->syncer();

	QVERIFY(!syncer.isDatabaseSyncing());
	QVERIFY(!ds->isDatabase());

	Json::Value dbJson;
	dbJson["dbType"] = "NOTCHOSEN";
	dbJson["interval"] = 1;

	syncer.startDatabaseSyncing(dbJson, false);
	QVERIFY(syncer.isDatabaseSyncing());
	QVERIFY(ds->isDatabase());
	QVERIFY(syncer.databaseJson() != Json::nullValue);

	syncer.stopDatabaseSyncing();
	QVERIFY(!syncer.isDatabaseSyncing());
	QVERIFY(!ds->isDatabase());
}

void TestAll::testSyncerSyncNowWithoutDataSource()
{
	DataSet * ds = _pkg->dataSet();
	QVERIFY(ds);

	DataSetSyncer & syncer = ds->syncer();

	QSignalSpy spy(&syncer, &DataSetSyncer::askUserForRelink);

	syncer.syncNow();

	QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
	QCOMPARE(spy.takeFirst()[0].toInt(), ds->id());
}

void TestAll::testSyncerMultipleStartStop()
{
	DataSet * ds = _pkg->dataSet();
	QVERIFY(ds);

	DataSetSyncer & syncer = ds->syncer();

	QTemporaryDir tempDir;
	QVERIFY(tempDir.isValid());
	QString path1 = tempDir.filePath("multi1.csv");
	QString path2 = tempDir.filePath("multi2.csv");

	auto makeFile = [&](const QString & p)
	{
		QFile f(p);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write("a\n1\n");
		f.close();
	};

	makeFile(path1);
	makeFile(path2);

	syncer.startFileSyncing(path1);
	QVERIFY(syncer.isFileSyncing());
	QCOMPARE(QString::fromStdString(ds->dataFilePath()), path1);

	syncer.startFileSyncing(path2);
	QVERIFY(syncer.isFileSyncing());
	QCOMPARE(QString::fromStdString(ds->dataFilePath()), path2);

	syncer.stopFileSyncing();
	QVERIFY(!syncer.isFileSyncing());

	syncer.startFileSyncing(path1);
	QVERIFY(syncer.isFileSyncing());
	QCOMPARE(QString::fromStdString(ds->dataFilePath()), path1);

	syncer.stopFileSyncing();
}


QTEST_MAIN(TestAll)
