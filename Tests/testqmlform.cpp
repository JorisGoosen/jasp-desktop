#include <iostream>
#include <QQmlEngine>
#include <QQmlContext>
#include "tempfiles.h"
#include "jasptheme.h"
#include "processinfo.h"
#include "testqmlform.h"
#include "datasetprovider.h"
#include "preferencesmodelbase.h"
#include "utilities/qmlutils.h"

TestForm::TestForm(QObject *parent)
	: QObject{parent}
{

}

void TestForm::applicationAvailable()
{
	// Initialization that only requires the QGuiApplication object to be available
}

void TestForm::qmlEngineAvailable(QQmlEngine *engine)
{
	TempFiles::init(ProcessInfo::currentPID());

	TempFiles::clearSessionDir();

	// Initialization requiring the QQmlEngine to be constructed
	engine->rootContext()->setContextProperty("myContextProperty", QVariant(true));

	QmlUtils::setGlobalPropertiesInQMLContext(engine->rootContext());

	static QStringList originalImportPaths = engine->importPathList();

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

void TestForm::cleanupTestCase()
{
	delete _theme;
	delete _prov;

	_theme = nullptr;
	_prefs = nullptr;
	_prov  = nullptr;
}


QUICK_TEST_MAIN_WITH_SETUP(qmlformtest, TestForm);

#include "testqmlform.moc"
