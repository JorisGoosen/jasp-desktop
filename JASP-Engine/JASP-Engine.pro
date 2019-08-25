
QT += core
QT -= gui

include(../JASP.pri)
BUILDING_JASP_ENGINE=true

CONFIG += c++11
linux:CONFIG += -pipe

DESTDIR   = ..
TARGET    = JASPEngine
CONFIG   += cmdline
CONFIG   -= app_bundle
TEMPLATE  = app

target.path  = $$INSTALLPATH
INSTALLS    += target

DEPENDPATH      = ..
PRE_TARGETDEPS += ../JASP-Common

LIBS += -L.. -l$$JASP_R_INTERFACE_NAME -lJASP-Common

include(../R_HOME.pri) #needed to build r-packages

windows {
  CONFIG(ReleaseBuild): LIBS += -llibboost_filesystem-vc141-mt-1_64 -llibboost_system-vc141-mt-1_64 -larchive.dll
  CONFIG(DebugBuild):   LIBS += -llibboost_filesystem-vc141-mt-gd-1_64 -llibboost_system-vc141-mt-gd-1_64 -larchive.dll
  LIBS           += -lole32 -loleaut32
  INCLUDEPATH    += ../../boost_1_64_0
  QMAKE_CXXFLAGS += -DBOOST_USE_WINDOWS_H -DNOMINMAX -DBOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED
}

unix: LIBS += -L$$_R_HOME/lib -lR

macx {
  LIBS        += -lboost_filesystem-clang-mt-1_64 -lboost_system-clang-mt-1_64 -larchive -lz
  INCLUDEPATH += ../../boost_1_64_0

  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-unused-local-typedef
  QMAKE_CXXFLAGS         += -Wno-c++11-extensions -Wno-c++11-long-long -Wno-c++11-extra-semi -stdlib=libc++
}

linux {
    LIBS += -larchive -lrt
    exists(/app/lib/*)	{ LIBS += -L/app/lib }
    LIBS += -lboost_filesystem -lboost_system
}

$$JASPTIMER_USED {
    windows:CONFIG(ReleaseBuild)    LIBS += -llibboost_timer-vc141-mt-1_64 -llibboost_chrono-vc141-mt-1_64
    windows:CONFIG(DebugBuild)      LIBS += -llibboost_timer-vc141-mt-gd-1_64 -llibboost_chrono-vc141-mt-gd-1_64
    linux:                          LIBS += -lboost_timer -lboost_chrono
    macx:                           LIBS += -lboost_timer-clang-mt-1_64 -lboost_chrono-clang-mt-1_64
}

INCLUDEPATH += $$PWD/../JASP-Common/

mkpath($$OUT_PWD/../R/library)

InstallJASPRPackage.commands        =  $${INSTALL_R_PKG_CMD_PREFIX}$$PWD/JASP$${INSTALL_R_PKG_CMD_POSTFIX}
InstallJASPgraphsRPackage.commands  =  $${INSTALL_R_PKG_CMD_PREFIX}$$PWD/JASPgraphs$${INSTALL_R_PKG_CMD_POSTFIX}

QMAKE_EXTRA_TARGETS += InstallJASPgraphsRPackage
POST_TARGETDEPS     += InstallJASPgraphsRPackage

QMAKE_EXTRA_TARGETS += InstallJASPRPackage
POST_TARGETDEPS     += InstallJASPRPackage

QMAKE_CLEAN += $$OUT_PWD/../R/library/*

SOURCES += \
    main.cpp \
    engine.cpp \
    rbridge.cpp \
    r_functionwhitelist.cpp

HEADERS += \
    engine.h \
    rbridge.h \
    r_functionwhitelist.h

DISTFILES += \
    JASPgraphs/DESCRIPTION \
    JASPgraphs/NAMESPACE \
    JASPgraphs/R/colorPalettes.R \
    JASPgraphs/R/compatability.R \
    JASPgraphs/R/convenience.R \
    JASPgraphs/R/customGeoms.R \
    JASPgraphs/R/descriptivesPlots.R \
    JASPgraphs/R/functions.R \
    JASPgraphs/R/getPrettyAxisBreaks.R \
    JASPgraphs/R/ggMatrixPlot.R \
    JASPgraphs/R/graphOptions.R \
    JASPgraphs/R/highLevelPlots.R \
    JASPgraphs/R/jaspLabelAxes.R \
    JASPgraphs/R/jaspScales.R \
    JASPgraphs/R/legendToPlotRatio.R \
    JASPgraphs/R/parseThis.R \
    JASPgraphs/R/PlotPieChart.R \
    JASPgraphs/R/PlotPizza.R \
    JASPgraphs/R/PlotPriorAndPosterior.R \
    JASPgraphs/R/plotQQnorm.R \
    JASPgraphs/R/PlotRobustnessSequential.R \
    JASPgraphs/R/printJASPgraphs.R \
    JASPgraphs/R/themeJasp.R \
    JASPgraphs/R/todo.R \
    JASPgraphs/man/colors.Rd \
    JASPgraphs/man/drawBFpizza.Rd \
    JASPgraphs/man/makeGridLines.Rd \
    JASPgraphs/man/parseThis.Rd \
    JASPgraphs/man/plotPieChart.Rd \
    JASPgraphs/man/PlotPriorAndPosterior.Rd \
    JASPgraphs/man/plotQQnorm.Rd \
    JASPgraphs/man/PlotRobustnessSequential.Rd \
    JASPgraphs/inst/examples/ex-colorPalettes.R \
    JASPgraphs/inst/examples/ex-PlotPieChart.R \
    JASP/DESCRIPTION \
    JASP/NAMESPACE \
    JASP/R/assignFunctionInPackage.R \
    JASP/R/base64.R \
    JASP/R/common.R \
    JASP/R/commonerrorcheck.R \
    JASP/R/commonmessages.R \
    JASP/R/commonMPR.R \
    JASP/R/distributionSamplers.R \
    JASP/R/exposeUs.R \
    JASP/R/friendlyConstructorFunctions.R \
    JASP/R/packagecheck.R \
    JASP/R/transformFunctions.R

