#include "tempfiles.h"
#include "testimport.h"
#include "processinfo.h"
#include "hardcodeddata.h"
#include "utilities/qutils.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
#include "data/importers/readstatimporter.h"

void TestImport::initTestCase()
{
	TempFiles::init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory
	_pkg = new DataSetPackage(this);
}

void TestImport::init()
{
	_pkg->reset();
}

void TestImport::testDebugCsv()
{
	CSVImporter		importer;

	importer.loadDataSet("../Resources/Data Sets/debug.csv", [](int i){ /*std::cout << i << " ";*/ });
	//std::cout << std::endl;

	DataSet * dataSet = _pkg->dataSet();

	QVERIFY2(dataSet,						"No dataset!");
	QVERIFY2(dataSet->columnCount()==31,	"Not enough columns!");
	QVERIFY2(dataSet->rowCount()==100,		"Not enough rows!");

	stringvec columnNames {"V1","contNormal","contGamma","contBinom","contExpon","contWide","contNarrow","contOutlier","contcor1","contcor2","facGender","facExperim","facFive","facFifty","facOutlier","debString","debMiss1","debMiss30","debMiss80","debMiss99","debBinMiss20","debNaN","debNaN10","debInf","debCollin1","debCollin2","debCollin3","debEqual1","debEqual2","debSame","unicode"};
	QCOMPARE(dataSet->getColumnNames(), columnNames);
	QCOMPARE(dataSet->jsonForCompare(), debugCsvJson());
}

QDir _TestLibrary()
{
	return QDir("../../jasp-desktop/Tests/TestLibrary/"); //This should probably be done better
}

void TestImport::testDebugReadStat()
{

	QDir readstatdir = _TestLibrary();
	readstatdir.cd("readstat");

	for(QFileInfo & i : readstatdir.entryInfoList(QDir::Filter::Files | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
		for(const std::string & ext : ReadStatImporter::extsSupported())
			if(i.fileName().endsWith("." + tq(ext)))
			{
				std::cout << "Testing " << i.absoluteFilePath() << std::endl;
				_pkg->reset();

				ReadStatImporter		importer(ext);
				importer.loadDataSet(fq(i.absoluteFilePath()), [](int i){});

				DataSet * dataSet = _pkg->dataSet();
				QVERIFY2(dataSet,						"No dataset!");

				Json::Value compareMe = dataSet->jsonForCompare();

				QString jsonFilePath = i.absoluteFilePath();
				jsonFilePath.replace(jsonFilePath.size() - (ext.size() + 1), ext.size() + 1, ".json");

				QFileInfo jsonFileIn(jsonFilePath);

				if(!jsonFileIn.exists())
				{
					std::cerr << "You should create " << jsonFilePath << std::endl;
					std::cerr << stringUtils::replaceBy(compareMe.toStyledString(), "\n", " ") << std::endl;

				}
				QVERIFY(jsonFileIn.exists());

				QFile jsonFile(jsonFilePath);

				jsonFile.open(QFile::OpenModeFlag::ReadOnly);

				std::string jsonTxt  = fq(jsonFile.readAll());

				Json::Reader parser;
				Json::Value  hardcoded;

				QVERIFY2(parser.parse(jsonTxt, hardcoded),	"Parsing json failed!");
				QVERIFY2(hardcoded == compareMe,			"Hardcoded json is different!");
			}
}



QTEST_MAIN(TestImport)
