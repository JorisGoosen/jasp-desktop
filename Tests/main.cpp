//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//


#include <QDir>

#include "utilities/application.h"
#include "utilities/settings.h"
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include <codecvt>
#include "appinfo.h"
#include <iostream>
#include "timers.h"
#include <QMessageBox>
#include "utilities/plotschemehandler.h"
#include "utilities/imgschemehandler.h"
#include <json/json.h>
#include "utilities/appdirs.h"

#ifdef linux
#include "utilities/qmlutils.h"
#endif

#ifdef _WIN32
#define QSTRING_FILE_ARG QString::fromLocal8Bit
#else
#define QSTRING_FILE_ARG QString::fromStdString
#endif



void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	const char *file	= context.file ? context.file : "";
	const char *function = context.function ? context.function : "";

	switch (type) {
	case QtWarningMsg:
		Log::log() << "Msg from Qt Warning: " << msg << " [" << file << ":" << context.line << ", " << function << "]" << std::endl;
		break;
	case QtCriticalMsg:
		Log::log() << "Msg from Qt Critical: " << msg << " [" << file << ":" << context.line << ", " << function << "]" << std::endl;
		break;
	case QtFatalMsg:
		Log::log() << "Msg from Qt Fatal: " << msg << " [" << file << ":" << context.line << ", " << function << "]" << std::endl;
		break;
	case QtDebugMsg:
	case QtInfoMsg:
		break;
	}
}

//Should do sort of the same setup as Desktop/main.cpp
int main(int argc, char *argv[])
{
	qInstallMessageHandler(qtMessageHandler);

	QCoreApplication::setOrganizationName("JASP");
	QCoreApplication::setOrganizationDomain("jasp-stats.org");
	QCoreApplication::setApplicationName("JASPTest");

	Dirs::setLocalAppdataDir(AppDirs::appData(false).toStdString());

	PlotSchemeHandler::createUrlScheme(); //Needs to be done *before* creating PlotSchemeHandler instance and also before QApplication is instantiated
	ImgSchemeHandler::createUrlScheme();

	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false); //To avoid weird splitterbehaviour with QML and a touchscreen

	#ifdef linux
		QmlUtils::configureQMLCacheDir();
	#endif

	QLocale::setDefault(QLocale(QLocale::English)); // make decimal points == . in at least R? Anyway, this has been here forever, ill just leave it.

	std::cout << "\nStarting JASPTest " << AppInfo::version.asString() << " from commit " << AppInfo::gitCommit << " and branch " << AppInfo::gitBranch << std::endl;

	JASPTIMER_START("JASP");

	QtWebEngineQuick::initialize(); // We can do this here and not in MainWindow::loadQML() (before QQmlApplicationEngine is instantiated) because that is called from a singleshot timer. And will only be executed once we enter a.exec() below!
	std::cout << "QtWebEngineQuick initialized" << std::endl;

	Application a(argc, argv);

	std::cout << "Application initialized" << std::endl;

	PlotSchemeHandler plotSchemeHandler; //Makes sure plots can still be loaded in webengine with Qt6
	ImgSchemeHandler  imgSchemeHandler;

	a.init("", false, false, 0, false, false, Json::nullValue, "");

	try
	{
		std::cout << "Entering eventloop" << std::endl;

		int exitCode = a.exec();
		JASPTIMER_STOP("JASP");
		JASPTIMER_PRINTALL();
		return exitCode;
	}
	catch(std::exception & e)
	{
		std::cerr << "Uncaught std::exception! Was: " << e.what() << "\n";
		return -1;
	}
	catch(...)
	{
		std::cerr << "Uncaught ???\n";
		return -1;
	}
}
