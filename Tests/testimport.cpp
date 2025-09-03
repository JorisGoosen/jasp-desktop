#include "tempfiles.h"
#include "testimport.h"
#include "processinfo.h"
#include "hardcodeddata.h"
#include "data/datasetpackage.h"
#include "data/importers/csvimporter.h"

void TestImport::initTestCase()
{
	TempFiles::init(ProcessInfo::currentPID()); // needed here so that the LRNAM can be passed the session directory
}

void TestImport::testDebugCsv()
{
	DataSetPackage		pkg(nullptr);
	CSVImporter		importer;

	importer.loadDataSet("../Resources/Data Sets/debug.csv", [](int i){ /*std::cout << i << " ";*/ });
	//std::cout << std::endl;

	QVERIFY2(pkg.dataSet(),						"No dataset!");
	QVERIFY2(pkg.dataSet()->columnCount()==31,	"Not enough columns!");
	QVERIFY2(pkg.dataSet()->rowCount()==100,	"Not enough rows!");
	stringvec columnNames {"V1","contNormal","contGamma","contBinom","contExpon","contWide","contNarrow","contOutlier","contcor1","contcor2","facGender","facExperim","facFive","facFifty","facOutlier","debString","debMiss1","debMiss30","debMiss80","debMiss99","debBinMiss20","debNaN","debNaN10","debInf","debCollin1","debCollin2","debCollin3","debEqual1","debEqual2","debSame","unicode"};
	QCOMPARE(pkg.dataSet()->getColumnNames(), columnNames);
	QCOMPARE(pkg.dataSet()->jsonForCompare(), debugCsvJson());
}



QTEST_MAIN(TestImport)
