#include "testall.h"
#include "tempfiles.h"
#include "processinfo.h"
#include "utilities/qutils.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
#include "data/importers/odsimporter.h"
#include "data/importers/excelimporter.h"
#include "data/importers/rdataimporter.h"
#include "data/importers/readstatimporter.h"

void TestAll::initTestCase()
{
	TempFiles::init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory
	_pkg = new DataSetPackage(this);
}

void TestAll::init()
{
	_pkg->reset();
}

void TestAll::cleanup()
{
	if(_importer)
		delete _importer;
	_importer = nullptr;


}

QDir _TestLibrary()
{
	return QDir("../../jasp-desktop/Tests/TestLibrary/"); //This should probably be done better
}

void TestAll::testDataImport()
{
	for(const QString & folder : _TestLibrary().entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
	{
		std::cerr << "Entering folder " << folder << std::endl;

		if(folder == "jasp")
			continue;

		QDir subDir(_TestLibrary());
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

		_importer = getImporter();

		QVERIFY2(_importer, "Getting importer failed...");

		for(QFileInfo & i : subDir.entryInfoList(QDir::Filter::Files | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
			if(i.suffix() != "json")
			{
				std::cerr << "Testing " << i.absoluteFilePath() << std::endl;
				_pkg->reset();

				_importer->loadDataSet(fq(i.absoluteFilePath()), [](int i){});

				DataSet * dataSet = _pkg->dataSet();
				QVERIFY2(dataSet,						"No dataset!");

				Json::Value compareMe = dataSet->jsonForCompare();

				QString jsonFilePath = i.absoluteFilePath(),
						ext			 = i.suffix();

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

			}

		delete _importer;

		_importer=nullptr;
	}
}



QTEST_MAIN(TestAll)
