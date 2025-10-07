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
	
	Json::Value compareMe = _data->jsonForCompare();

	QString jsonFilePath = dataFileAbsolutePath,
			ext			 = QFileInfo(dataFileAbsolutePath).suffix();

	jsonFilePath.replace(jsonFilePath.size() - (ext.size() + 1), ext.size() + 1, ".json");

	QFileInfo jsonFileIn(jsonFilePath);

	if(!jsonFileIn.exists())
	{
		std::cerr << "Json does not exist yet, creating it now!" << std::endl;
		QFile jsonFile(jsonFilePath);
		jsonFile.open(QFile::OpenModeFlag::WriteOnly);
		jsonFile.write(stringUtils::replaceBy(compareMe.toStyledString(), "\n", " ").c_str());
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

	delete _importer;
	_importer = nullptr;

	DatabaseInterface::singleton()->close();
	DatabaseInterface::singleton()->closeInterfaces();
	delete _pkg;
	_pkg = nullptr;
}


QTEST_MAIN(TestDebugData)
