QT -= core gui

JASP_BUILDROOT_DIR = $$OUT_PWD/../..
include(../../JASP.pri)
include(../../R_HOME.pri)

TEMPLATE = aux
CONFIG -= app_bundle

MODULE_DIR  = $$PWD
MODULE_NAME = Audit
include(../InstallModule.pri)

MODULE_NAME = BAIN
include(../InstallModule.pri)

MODULE_NAME = MixedModels
include(../InstallModule.pri)

MODULE_NAME = Network
include(../InstallModule.pri)

MODULE_NAME = SEM
include(../InstallModule.pri)

MODULE_NAME = MachineLearning
include(../InstallModule.pri)

MODULE_NAME = SummaryStatistics
include(../InstallModule.pri)

MODULE_NAME = MetaAnalysis
include(../InstallModule.pri)

DISTFILES += \
    Audit/inst/description.json \
    Audit/inst/help/img/cellSampling.png \
    Audit/inst/help/img/fixedIntervalSampling.png \
    Audit/inst/help/img/randomSampling.png \
    Audit/inst/icons/audit-module.svg \
    Audit/inst/icons/audit-planning.svg \
    Audit/inst/icons/audit-workflow.svg \
    Audit/inst/help/bayesianaudit.md \
    Audit/inst/help/bayesianplanning.md \
    Audit/inst/help/cellSampling.md \
    Audit/inst/help/classicalaudit.md \
    Audit/inst/help/classicalplanning.md \
    Audit/inst/help/explanatoryText.md \
    Audit/inst/help/fixedIntervalSampling.md \
    Audit/inst/help/monetaryUnitSampling.md \
    Audit/inst/help/randomSampling.md \
    Audit/inst/help/recordSampling.md \
    Audit/R/bayesianAudit.R \
    Audit/R/bayesianPlanning.R \
    Audit/R/classicalAudit.R \
    Audit/R/classicalPlanning.R \
    Audit/R/commonAudit.R \
    Audit/R/commonAuditSampling.R \
    Audit/R/commonBayesianAuditMethods.R \
    Audit/R/commonClassicalAuditMethods.R \
    Audit/inst/qml/bayesianAudit.qml \
    Audit/inst/qml/bayesianPlanning.qml \
    Audit/inst/qml/classicalAudit.qml \
    Audit/inst/qml/classicalPlanning.qml \
    BAIN/inst/description.json \
    BAIN/inst/icons/analysis-bain-anova.svg \
    BAIN/inst/icons/analysis-bain-regression.svg \
    BAIN/inst/icons/analysis-bain-ttest.svg \
    BAIN/inst/icons/bain-module.svg \
    BAIN/inst/help/BainRegressionLinearBayesian.md \
    BAIN/inst/help/BainTTestBayesianIndependentSamples.md \
    BAIN/inst/help/BainTTestBayesianOneSample.md \
    BAIN/inst/help/BainTTestBayesianPairedSamples.md \
    BAIN/R/bainancovabayesian.R \
    BAIN/R/bainanovabayesian.R \
    BAIN/R/bainregressionlinearbayesian.R \
    BAIN/R/bainttestbayesianindependentsamples.R \
    BAIN/R/bainttestbayesianonesample.R \
    BAIN/R/bainttestbayesianpairedsamples.R \
    BAIN/inst/qml/BainAncovaBayesian.qml \
    BAIN/inst/qml/BainAnovaBayesian.qml \
    BAIN/inst/qml/BainRegressionLinearBayesian.qml \
    BAIN/inst/qml/BainTTestBayesianIndependentSamples.qml \
    BAIN/inst/qml/BainTTestBayesianOneSample.qml \
    BAIN/inst/qml/BainTTestBayesianPairedSamples.qml \
    MachineLearning/inst/description.json \
    MachineLearning/R/commonMachineLearningClassification.R \
    MachineLearning/R/commonMachineLearningClustering.R \
    MachineLearning/R/commonMachineLearningRegression.R \
    MachineLearning/R/mlClassificationBoosting.R \
    MachineLearning/R/mlClassificationKnn.R \
    MachineLearning/R/mlClassificationLda.R \
    MachineLearning/R/mlClassificationRandomForest.R \
    MachineLearning/R/mlClusteringDensityBased.R \
    MachineLearning/R/mlClusteringFuzzyCMeans.R \
    MachineLearning/R/mlClusteringHierarchical.R \
    MachineLearning/R/mlClusteringKMeans.R \
    MachineLearning/R/mlClusteringRandomForest.R \
    MachineLearning/R/mlRegressionBoosting.R \
    MachineLearning/R/mlRegressionKnn.R \
    MachineLearning/R/mlRegressionRandomForest.R \
    MachineLearning/R/mlRegressionRegularized.R \
    MachineLearning/inst/qml/common/DataSplit.qml \
    MachineLearning/inst/qml/mlClassificationBoosting.qml \
    MachineLearning/inst/qml/mlClassificationKnn.qml \
    MachineLearning/inst/qml/mlClassificationLda.qml \
    MachineLearning/inst/qml/mlClassificationRandomForest.qml \
    MachineLearning/inst/qml/mlClusteringDensityBased.qml \
    MachineLearning/inst/qml/mlClusteringFuzzyCMeans.qml \
    MachineLearning/inst/qml/mlClusteringHierarchical.qml \
    MachineLearning/inst/qml/mlClusteringKMeans.qml \
    MachineLearning/inst/qml/mlClusteringRandomForest.qml \
    MachineLearning/inst/qml/mlRegressionBoosting.qml \
    MachineLearning/inst/qml/mlRegressionKnn.qml \
    MachineLearning/inst/qml/mlRegressionRandomForest.qml \
    MachineLearning/inst/qml/mlRegressionRegularized.qml \
    MachineLearning/inst/help/mlclassificationboosting.md \
    MachineLearning/inst/help/mlclassificationknn.md \
    MachineLearning/inst/help/mlclassificationlda.md \
    MachineLearning/inst/help/mlclassificationrandomforest.md \
    MachineLearning/inst/help/mlclusteringdensitybased.md \
    MachineLearning/inst/help/mlclusteringfuzzycmeans.md \
    MachineLearning/inst/help/mlclusteringhierarchical.md \
    MachineLearning/inst/help/mlclusteringkmeans.md \
    MachineLearning/inst/help/mlclusteringrandomforest.md \
    MachineLearning/inst/help/mlregressionboosting.md \
    MachineLearning/inst/help/mlregressionknn.md \
    MachineLearning/inst/help/mlregressionrandomforest.md \
    MachineLearning/inst/help/mlregressionregularized.md \
    MachineLearning/inst/icons/analysis-ml-classification.svg \
    MachineLearning/inst/icons/analysis-ml-clustering.svg \
    MachineLearning/inst/icons/analysis-ml-regression.svg \
    MachineLearning/inst/icons/analysis-ml-ribbon.svg \
    MetaAnalysis/inst/description.json \
    MetaAnalysis/R/classicalmetaanalysis.R \
    MetaAnalysis/R/emmeans.rma.R \
    MetaAnalysis/R/multilevelmetaanalysis.R \
    MetaAnalysis/R/quick.influence.R \
    MetaAnalysis/inst/icons/meta-analysis.svg \
    MetaAnalysis/inst/qml/ClassicalMetaAnalysis.qml \
    MetaAnalysis/inst/qml/MultilevelMetaAnalysis.qml \
    MetaAnalysis/inst/help/classicalmetaanalysis.md \
    MixedModels/inst/description.json \
    MixedModels/inst/icons/analysis-descriptives.svg \
    MixedModels/R/linearmixedmodels.R \
    MixedModels/inst/qml/LinearMixedModels.qml \
    Network/inst/description.json \
    Network/R/networkanalysis.R \
    Network/inst/qml/NetworkAnalysis.qml \
    Network/inst/help/networkanalysis.md \
    Network/inst/icons/analysis-network.svg \
    SEM/inst/description.json \
    SEM/R/mediationanalysis.R \
    SEM/R/semsimple.R \
    SEM/inst/qml/MediationAnalysis.qml \
    SEM/inst/qml/SEMSimple.qml \
    SEM/inst/help/mediationanalysis.md \
    SEM/inst/help/semsimple.md \
    SEM/inst/icons/sem-latreg.svg \
    SummaryStatistics/inst/description.json \
    SummaryStatistics/inst/icons/analysis-bayesian-crosstabs.svg \
    SummaryStatistics/inst/icons/analysis-bayesian-regression.svg \
    SummaryStatistics/inst/icons/analysis-bayesian-ttest.svg \
    SummaryStatistics/inst/help/summarystatsbinomialtestbayesian.md \
    SummaryStatistics/inst/help/summarystatscorrelationbayesianpairs.md \
    SummaryStatistics/inst/help/summarystatsregressionlinearbayesian.md \
    SummaryStatistics/inst/help/summarystatsttestbayesianindependentsamples.md \
    SummaryStatistics/inst/help/summarystatsttestbayesianonesample.md \
    SummaryStatistics/inst/help/summarystatsttestbayesianpairedsamples.md \
    SummaryStatistics/R/commonsummarystats.R \
    SummaryStatistics/R/commonsummarystatsttestbayesian.R \
    SummaryStatistics/R/summarystatsbinomialtestbayesian.R \
    SummaryStatistics/R/summarystatscorrelationbayesianpairs.R \
    SummaryStatistics/R/summarystatsregressionlinearbayesian.R \
    SummaryStatistics/R/summarystatsttestbayesianindependentsamples.R \
    SummaryStatistics/R/summarystatsttestbayesianonesample.R \
    SummaryStatistics/R/summarystatsttestbayesianpairedsamples.R \
    SummaryStatistics/inst/qml/SummaryStatsBinomialTestBayesian.qml \
    SummaryStatistics/inst/qml/SummaryStatsCorrelationBayesianPairs.qml \
    SummaryStatistics/inst/qml/SummaryStatsRegressionLinearBayesian.qml \
    SummaryStatistics/inst/qml/SummaryStatsTTestBayesianIndependentSamples.qml \
    SummaryStatistics/inst/qml/SummaryStatsTTestBayesianOneSample.qml \
    SummaryStatistics/inst/qml/SummaryStatsTTestBayesianPairedSamples.qml

SUBDIRS += \
    Extra.pro


