#include "tempfiles.h"
#include "testimport.h"
#include "processinfo.h"
#include "utilities/qutils.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"
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

QDir _TestLibrary()
{
	return QDir("../../jasp-desktop/Tests/TestLibrary/"); //This should probably be done better
}

void TestAll::testDataImport()
{
	for(QFileInfo & dirInfo : _TestLibrary().entryInfoList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
	{

		std::cout << "Entering directory " << dirInfo.absolutePath() << std::endl;

		for(QFileInfo & i : dirInfo.dir().entryInfoList(QDir::Filter::Files | QDir::Filter::NoDotAndDotDot | QDir::Filter::NoSymLinks))
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
}



QTEST_MAIN(TestAll)
