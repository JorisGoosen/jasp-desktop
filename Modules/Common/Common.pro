QT -= core gui

JASP_BUILDROOT_DIR = $$OUT_PWD/../..
include(../../JASP.pri)
include(../../R_HOME.pri)

TEMPLATE = aux
CONFIG -= app_bundle

MODULE_DIR  = $$PWD
MODULE_NAME = Descriptives
include(../InstallModule.pri)

MODULE_NAME = ANOVA
include(../InstallModule.pri)

MODULE_NAME = Factor
include(../InstallModule.pri)

MODULE_NAME = Frequencies
include(../InstallModule.pri)

MODULE_NAME = Regression
include(../InstallModule.pri)

MODULE_NAME = T-Tests
include(../InstallModule.pri)

DISTFILES += \
    ANOVA/inst/help/anova.md \
    ANOVA/inst/help/ancova.md \
    ANOVA/inst/help/manova.md \
    ANOVA/inst/description.json \
    ANOVA/inst/help/anovabayesian.md \
    ANOVA/inst/help/ancovabayesian.md \
    ANOVA/inst/help/BainAnovaBayesian.md \
    ANOVA/inst/help/BainAncovaBayesian.md \
    ANOVA/inst/help/repeatedmeasuresanova.md \
    ANOVA/inst/icons/analysis-bayesian-anova.svg \
    ANOVA/inst/icons/analysis-classical-anova.svg \
    ANOVA/inst/help/anovarepeatedmeasuresbayesian.md \
    ANOVA/inst/help/repeatedmeasuresanovabayesian.md \
    ANOVA/inst/qml/AnovaRepeatedMeasuresBayesian.qml \
    ANOVA/R/anovarepeatedmeasuresbayesian.R \
    ANOVA/R/anovarepeatedmeasures.R \
    ANOVA/R/commonAnovaBayesian.R \
    ANOVA/R/ancovamultivariate.R \
    ANOVA/R/anovamultivariate.R \
    ANOVA/R/ancovabayesian.R \
    ANOVA/R/anovabayesian.R \
    ANOVA/R/anovaoneway.R \
    ANOVA/R/ancova.R \
    ANOVA/R/anova.R \
    ANOVA/R/manova.R \
    ANOVA/inst/qml/Anova.qml \
    ANOVA/inst/qml/Manova.qml \
    ANOVA/inst/qml/Ancova.qml \
    ANOVA/inst/qml/AnovaBayesian.qml \
    ANOVA/inst/qml/AncovaBayesian.qml \
    ANOVA/inst/qml/AnovaRepeatedMeasures.qml \
    Descriptives/inst/help/reliabilityanalysis.md \
    Descriptives/inst/icons/analysis-descriptives.svg \
    Descriptives/inst/qml/ReliabilityAnalysis.qml \
    Descriptives/inst/help/descriptives.md \
    Descriptives/inst/qml/Descriptives.qml \
    Descriptives/R/reliabilityanalysis.R \
    Descriptives/inst/description.json \
    Descriptives/R/descriptives.R \
    Descriptives/DESCRIPTION \
    Descriptives/NAMESPACE \
    Factor/inst/description.json \
    Factor/inst/qml/FactorsForm.qml \
    Factor/inst/qml/FactorsList.qml \
    Factor/R/exploratoryfactoranalysis.R \
    Factor/R/principalcomponentanalysis.R \
    Factor/R/confirmatoryfactoranalysis.R \
    Factor/inst/icons/analysis-classical-sem.svg \
    Factor/inst/help/exploratoryfactoranalysis.md \
    Factor/inst/qml/ExploratoryFactorAnalysis.qml \
    Factor/inst/help/principalcomponentanalysis.md \
    Factor/inst/help/confirmatoryfactoranalysis.md \
    Factor/inst/qml/PrincipalComponentAnalysis.qml \
    Factor/inst/qml/ConfirmatoryFactorAnalysis.qml \
    Frequencies/inst/help/contingencytablesbayesian.md \
    Frequencies/inst/icons/analysis-classical-crosstabs.svg \
    Frequencies/inst/qml/RegressionLogLinearBayesian.qml \
    Frequencies/inst/qml/ContingencyTablesBayesian.qml \
    Frequencies/inst/help/multinomialtestbayesian.md \
    Frequencies/inst/help/binomialtestbayesian.md \
    Frequencies/inst/help/contingencytables.md \
    Frequencies/inst/help/multinomialtest.md \
    Frequencies/inst/help/abtestbayesian.md \
    Frequencies/inst/help/binomialtest.md \
    Frequencies/inst/description.json \
    Frequencies/R/abtestbayesian.R \
    Frequencies/R/binomialtest.R \
    Frequencies/R/multinomialtest.R \
    Frequencies/R/contingencytables.R \
    Frequencies/R/regressionloglinear.R \
    Frequencies/R/binomialtestbayesian.R \
    Frequencies/R/contingencytablesbayesian.R \
    Frequencies/R/regressionloglinearbayesian.R \
    Frequencies/inst/qml/BinomialTestBayesian.qml \
    Frequencies/inst/qml/MultinomialTestBayesian.qml \
    Frequencies/inst/qml/RegressionLogLinear.qml \
    Frequencies/inst/qml/ContingencyTables.qml \
    Frequencies/inst/qml/MultinomialTest.qml \
    Frequencies/R/multinomialtestbayesian.R \
    Frequencies/inst/qml/ABTestBayesian.qml \
    Frequencies/inst/qml/BinomialTest.qml \
    Regression/inst/description.json \
    Regression/inst/help/correlation.md \
    Regression/inst/help/regressionlinear.md \
    Regression/inst/help/regressionlogistic.md \
    Regression/inst/help/regressionloglinear.md \
    Regression/inst/help/correlationbayesian.md \
    Regression/inst/help/correlationbayesianpairs.md \
    Regression/inst/help/regressionlinearbayesian.md \
    Regression/inst/help/regressionloglinearbayesian.md \
    Regression/inst/icons/analysis-classical-regression.svg \
    Regression/inst/qml/CorrelationBayesianPairs.qml \
    Regression/inst/qml/RegressionLinearBayesian.qml \
    Regression/inst/qml/CorrelationBayesian.qml \
    Regression/inst/qml/RegressionLogistic.qml \
    Regression/inst/qml/RegressionLinear.qml \
    Regression/R/correlationbayesianpairs.R \
    Regression/R/regressionlinearbayesian.R \
    Regression/inst/qml/Correlation.qml \
    Regression/R/correlationbayesian.R \
    Regression/R/regressionlogistic.R \
    Regression/R/regressionlinear.R \
    Regression/R/correlation.R \
    Regression/R/massStepAIC.R \
    Regression/R/commonglm.R \
    T-Tests/R/commonTTest.R \
    T-Tests/inst/description.json \
    T-Tests/R/commonbayesianttest.R \
    T-Tests/inst/help/ttestonesample.md \
    T-Tests/inst/help/ttestpairedsamples.md \
    T-Tests/R/ttestbayesianindependentsamples.R \
    T-Tests/inst/icons/analysis-classical-ttest.svg \
    T-Tests/inst/help/ttestbayesianindependentsamples.md \
    T-Tests/inst/help/ttestbayesianpairedsamples.md \
    T-Tests/inst/help/ttestbayesianonesample.md \
    T-Tests/inst/help/ttestindependentsamples.md \
    T-Tests/R/informedbayesianttestfunctions.R \
    T-Tests/R/ttestbayesianpairedsamples.R \
    T-Tests/R/ttestbayesianonesample.R \
    T-Tests/R/ttestindependentsamples.R \
    T-Tests/R/ttestonesample.R \
    T-Tests/R/ttestpairedsamples.R \
    T-Tests/R/ttestplotfunctions.R \
    T-Tests/inst/qml/TTestBayesianOneSample.qml \
    T-Tests/inst/qml/TTestBayesianPairedSamples.qml \
    T-Tests/inst/qml/TTestBayesianIndependentSamples.qml \
    T-Tests/inst/qml/TTestIndependentSamples.qml \
    T-Tests/inst/qml/TTestPairedSamples.qml \
    T-Tests/inst/qml/TTestOneSample.qml \
    Frequencies/DESCRIPTION \
    Frequencies/NAMESPACE
