#include <iostream>
#include <QQmlEngine>
#include <QQmlContext>
#include "tempfiles.h"
#include "jasptheme.h"
#include "processinfo.h"
#include "testqml.h"
#include "datasetprovider.h"
#include "preferencesmodelbase.h"
#include "utilities/qmlutils.h"

TestQml::TestQml(QObject *parent)
	: QObject{parent}
{

}

void TestQml::applicationAvailable()
{
	// Initialization that only requires the QGuiApplication object to be available
}

void TestQml::qmlEngineAvailable(QQmlEngine *engine)
{

	TempFiles::init(ProcessInfo::currentPID());

	TempFiles::clearSessionDir();

	// Initialization requiring the QQmlEngine to be constructed
	engine->rootContext()->setContextProperty("myContextProperty", QVariant(true));

	static QStringList originalImportPaths = engine->importPathList();

	QmlUtils::setGlobalPropertiesInQMLContext(engine->rootContext());

	QStringList newImportPaths = originalImportPaths;

	newImportPaths.append(":/jasp-stats.org/imports");
	newImportPaths.append("qrc:///components");

	engine->setImportPathList(newImportPaths);

	_theme = new JaspTheme();
	_prefs = new PreferencesModelBase(engine);

	engine->rootContext()->setContextProperty("jaspTheme",			_theme);
	engine->rootContext()->setContextProperty("preferencesModel",	_prefs);

	_prov = DataSetProvider::getProvider(false, false);

	_prov->absorbInfo(VariableInfo::DataSetValues, "TestColumn", 0, QVariantList({1,2,3,4,5}));
}

void TestQml::cleanupTestCase()
{
	delete _theme;
	delete _prov;

	_theme = nullptr;
	_prefs = nullptr;
	_prov  = nullptr;
}

QUICK_TEST_MAIN_WITH_SETUP(qmltest, TestQml);

#include "testqml.moc"
