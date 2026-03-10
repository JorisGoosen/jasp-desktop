#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QTimer>
#include "dataset.h"
#include "datasetbasenode.h"
#include "databaseinterface.h"

class Workspace : public DataSetBaseNode
{
	Q_OBJECT
	
	Q_PROPERTY(bool				dataMode			READ dataMode			WRITE setDataMode			NOTIFY dataModeChanged		)
	Q_PROPERTY(bool				showRSyntax			READ showRSyntax		WRITE setShowRSyntax		NOTIFY showRSyntaxChanged	)
	Q_PROPERTY(DataSet		*	shownDataSet		READ shownDataSet									NOTIFY shownDataSetChanged	)
	Q_PROPERTY(Column		*	shownColumn			READ shownColumn		WRITE setShownColumn		NOTIFY shownColumnChanged	)
	Q_PROPERTY(Filter		*	shownFilter			READ shownFilter		WRITE setShownFilter		NOTIFY shownFilterChanged	)
	Q_PROPERTY(VariableInfo *	varInfo				READ varInfo										CONSTANT					)
	
	// Emit signals also in refresh
	
public:
	explicit Workspace(QObject *parent = nullptr);
	~Workspace();
	
			DatabaseInterface	 &	db();
	const	DatabaseInterface	 &	db() const;
	
			bool					dataMode()				const	{ return _dataMode;		}
			bool					showRSyntax()			const	{ return _showRSyntax;	}
			
			int						rowCount(		const QModelIndex &parent = QModelIndex())										const	override { return _dataSets.size(); }
			int						columnCount(	const QModelIndex &parent = QModelIndex())										const	override { return 1; }
			QVariant				data(			const QModelIndex &index, int role = Qt::DisplayRole)							const	override;
			
			VariableInfo		*	varInfo() const { return _varInfo; }
			
			void					setDataMode(		bool mode);
			void					setShowRSyntax(		bool showRSyntax);
			
			void					dbLoad(std::function<void(float)> progressCallback = [](float){}, Version doUpgradeFrom = Version());
			void					dbUpdate();
			void					dbDelete();
			
			bool					checkForUpdates();
			
				
			DataSet				*	shownDataSet()	const;
			DataSets				dataSets()		const;
			DataSet				*	dataSetById(int id) const;
			Filter				*	filterById(int id) const;
			
			Column				*	shownColumn() const;
			Filter				*	shownFilter() const;
			void					setShownColumn(Column *newShownColumn);
			void					setShownFilter(Filter * newShownFilter);
			void					initializeComputedColumns();
	static	Workspace			*	singleton() { return _singleton; }
	
	
public slots:
			void					refresh();
			DataSet				*	createDataSet();
			Column				*	createComputedColumn(const std::string & name, int dataSetId, int analysisId = -1, columnType type = columnType::unknown, computedColumnType desiredType = computedColumnType::analysis);
			void					setShownDataSet(QString	  name);
			void					setShownDataSet(DataSet * dataSet);
			void					setShownDataSet(int		  dataSetId);
			void					deleteShownDataSet();
			void					showFilter(int id);
			void					onShownFilterChanged(DataSet * data);
			void					refreshAllCompCols(Filter * f);
			void					updateComputedColumnDependenciesForAnalysis(int analysisId, const stringset & usedVariables);
			
signals:
			void					filterByNameDone(int dataSetID, const QString & name, const QString & error);
			void					dataModeChanged(bool dataMode);
			void					showRSyntaxChanged(bool showIt);
			void					shownDataSetChanged(DataSet * dataSet);
			void					shownFilterChanged();
			void					manualEditMade(); 
			void					datasetChanged(				int						dataSetId,
																QStringList				changedColumns,
																QStringList				missingColumns,
																QMap<QString, QString>	changeNameColumns,
																bool					rowCountChanged,
																bool					hasNewColumns); 
			void					labelsReordered(			QString columnName);
			void					labelFilterChanged();
			QString					askPassword(	QString title, QString message);
			bool					showYesNo(		QString title, QString message);
			void					allFiltersReset();
			void					showWarning(						QString title, QString msg);
			void					descriptionChanged();
			void					dataFileChanged();
			void					databaseJsonChanged();
			void					dataFileSynchChanged();
			void					dataTimestampChanged();
			void					columnsLabelFilteredCountChanged();
			void					refreshAllAnalyses(Filter * f);
			void					runComputedColumn(int dataSetid, QString columnName, QString code, enum columnType columnType);
			void					sendFilter(			int dataSetID, const QString & generatedFilter, const QString & filter);
			void					sendFilterByName(	int dataSetID, const QString & name, const QString & module = "*");
			void					filtersCountChanged();
			void					enginesPrepareForData();
			void					enginesReceiveNewData();
			void					enableModified();
			void					dataSetSynchingStart(DataSet *);
			void					dataSetSynchingDone(DataSet * );
			void					synchingStart();
			void					synchingDone();
			void					shownColumnChanged();
			void					checkForDependentAnalyses(Column * column);
			void					showAnalysis(			int			analysisId);
			void					emptyValuesChanged();
			
	
			
private:
	std::map<int,DataSet*>			_dataSets;
	DataSet						*	_shownDataSet			= nullptr;
	VariableInfo				*	_varInfo				= nullptr;
	bool							_showRSyntax			= false,
									_dataMode				= false;
	QTimer							_syncher;
	static Workspace			*	_singleton;
	
};

#endif // WORKSPACE_H
