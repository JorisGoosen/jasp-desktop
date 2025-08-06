#ifndef DATASET_H
#define DATASET_H

#include "datasetbasenode.h"
#include "column.h"
#include "filter.h"
#include "emptyvalues.h"
#include "version.h"

class DataSet : public DataSetBaseNode
{
	Q_OBJECT
	
	//Would be nice to have EmptyValuesQ also and make it available as a property here
	Q_PROPERTY(QString	description				READ descriptionQ			WRITE setDescriptionQ		NOTIFY descriptionChanged			)
	Q_PROPERTY(QString	dataFile				READ dataFileQ				WRITE setDataFileQ			NOTIFY dataFileChanged				)
	Q_PROPERTY(QString	databaseJson			READ databaseJsonQ			WRITE setDatabaseJsonQ		NOTIFY databaseJsonChanged			)
	Q_PROPERTY(bool		dataFileSynch			READ dataFileSynchQ			WRITE setDataFileSynchQ		NOTIFY dataFileSynchChanged			)
	Q_PROPERTY(long		dataFileTimestamp		READ dataFileTimestamp		WRITE setDataTimestamp		NOTIFY dataTimestampChanged			)

	Q_PROPERTY(int		columnsFilteredCount	READ columnsFilteredCount								NOTIFY columnsFilteredCountChanged	)
	
	friend Column;
	
public:
	typedef 	std::map<std::string,columnType> colTypeMap;
	
							DataSet(int index = -1); ///< index==-1: create a new dataSet, >0: load that dataSet, 0: do nothing
							~DataSet();
	
			Filter		*	shownFilter()		const 		{ return	_filter;	}
			Columns		&	columns()			const		{ return	const_cast<Columns&>(_columns);	}
    const	EmptyValues *	emptyValues()       const		{ return	_emptyValues; }
			EmptyValues *	emptyValues()					{ return	_emptyValues; }

			Column		*	column(		const std::string & name);
			Column		*	column(		const QString & name);
			Column		*	column(		size_t				columnIndex);

			Column		*	operator[](	size_t				columnIndex)	{ return column(columnIndex); }
			Column		*	operator[](	const std::string &	columnName)		{ return column(columnName); }
	
			int				id()					const { return _dataSetID;				}
			bool			dataFileSynch()			const { return _dataFileSynch;			}
	const	std::string &	dataFilePath()			const { return _dataFilePath;			}
			int				dataFileTimestamp()		const { return _dataFileTimestamp;		}
	const	std::string &	databaseJson()			const { return _databaseJson;			}
			bool			writeBatchedToDB()		const { return _writeBatchedToDBDepth;		}
			void			batchColumnHadChange(Column *col);
			
			int				rowCount(		const QModelIndex &parent = QModelIndex())										const	override;
			int				columnCount(	const QModelIndex &parent = QModelIndex())										const	override;
			QVariant		data(			const QModelIndex &index, int role = Qt::DisplayRole)							const	override;
			bool			setData(		const QModelIndex &index, const QVariant &value, int role)								override;
			QVariant		headerData(		int section, Qt::Orientation orientation, int role = Qt::DisplayRole )			const	override;
			Qt::ItemFlags	flags(			const QModelIndex &index)														const	override;
			
			bool			insertRows(		int row,		int count, const QModelIndex & aparent = QModelIndex())					override;
			bool			insertColumns(	int column,		int count, const QModelIndex & aparent = QModelIndex())					override;
			bool			removeRows(		int row,		int count, const QModelIndex & aparent = QModelIndex())					override;
			bool			removeColumns(	int column,		int count, const QModelIndex & aparent = QModelIndex())					override;

			int				columnsFilteredCount()	const;

			void			dbCreate();
			void			dbUpdate();
			void			dbLoad(int index = -1, std::function<void(float)> progressCallback = [](float){}, Version doUpgradeFrom = Version());
			void			dbDelete();

			void			beginBatchedToDB();
			void			endBatchedToDB(std::function<void(float)> progressCallback = [](float){}, Columns columns={});
			void			endBatchedToDB(Columns columns) { endBatchedToDB([](float){}, columns); }
			
			void			removeColumn(	const	std::string &	name	);
			void			removeColumn(			size_t			index	);
			void			removeColumnById(		size_t			id		);
			void			insertColumn(			size_t			index,	bool alterDataSetTable = true);
			Column		*	newColumn(		const	std::string &	name);
			int				getColumnIndex(	const	std::string &	name	) const;
			int				columnIndex(	const	Column		*	col		) const;
			void			columnsReorder(			stringvec		order	); ///< Expects a sane order vector, with or without computed columns
			void			columnRefreshed(Column * column);

			bool			allColumnsPassFilter()					const;
			
			std::string		freeNewColumnName(size_t startHere)																const;
			bool			isColumnNameFree(const std::string & name)														const;
			
			QString			descriptionQ()		const;
			QString			dataFileQ()			const;
			QString			databaseJsonQ()		const;
			bool			dataFileSynchQ()	const;
			long			dataTimestamp()		const;
			
			
			
			void			setDescriptionQ(	const QString &	newDescription);
			void			setDataFileQ(		const QString &	newDataFile);
			void			setDatabaseJsonQ(	const QString &	newDatabaseJson);
			void			setDataFileSynchQ(	bool			newDataFileSynch);
			void			setDataFileAndTimeStamp(const std::string &dataFilePath, long timestamp);
			
			Column *		connectNewColumn(Column *newColumn);
			Filter *		connectNewFilter(Filter *newFilter);


			bool			dataFileCanHaveLabels() const;
			void			resetAllFilters();

			void			resetFilterCounters();
			void			resetVariableTypes(int thresholdScale);
			

			size_t			getMaximumColumnWidthInCharacters(size_t columnIndex) const;
			stringvec		getColumnNames();
			colTypeMap		getColumnTypesMap();
			
			bool			dataMode()				const	{ return _dataMode; }
			void			setDataMode(bool mode);

			void			setDataFile(		const std::string & dataFilePath);
			void			setDataTimestamp(	long timestamp);
			void			setDatabaseJson(	const std::string & databaseJson);
			void			setDataFileSynch(	bool synchronizing);
			void			emitColumnChanged(		const QString		& name);

			void			setColumnCount(	size_t colCount);
			void			setRowCount(	size_t rowCount);

			void			incRevision() override;
			bool			checkForUpdates(stringvec * colsChanged = nullptr, stringvec * colsRemoved = nullptr, bool * newColumns = nullptr, bool * rowCountChanged = nullptr);

			const Columns &	computedColumns() const;
			
			void			loadOldComputedColumnsJson(const Json::Value & json); ///< Should act the same as the old ComputedColumns::fromJson() to allow loading "older jaspfiles"
			stringset		findUsedColumnNames(std::string searchThis);

			DatabaseInterface	 &	db();
	const	DatabaseInterface	 &	db() const;
	
			void					setEmptyValuesJson(			const Json::Value & emptyValues, bool updateDB = true);
			
	const	stringset			&	workspaceEmptyValues()															const	{ return _emptyValues->emptyStrings();								}
	const	std::string			&	description()																	const	{ return _description; }
			void					setWorkspaceEmptyValues(	const stringset& values);
			void					setDescription(				const std::string& desc);
			void					updateLabelsPostLocaleChange();
			Json::Value				jsonForCompare() const;

signals:	//These should all still be connected to DataSetPackage or such
			void				manualEditMade(); 
			void				datasetChanged(				QStringList				changedColumns,
															QStringList				missingColumns,
															QMap<QString, QString>	changeNameColumns,
															bool					rowCountChanged,
															bool					hasNewColumns); 
			void				labelsReordered(			QString columnName);
			void				columnTypeChanged(			QString columnName);
			void				labelFilterChanged();
			
			void				allFiltersReset();
			void				showWarning(						QString title, QString msg);
			void				columnsBeingRemoved(				int columnIndex, int count);
			void				descriptionChanged();
			void				dataFileChanged();
			void				databaseJsonChanged();
			void				dataFileSynchChanged();
			void				dataTimestampChanged();
			void				columnsFilteredCountChanged();
			void				refreshAllAnalyses();
			void				refreshAllCompCols();
			void				dataModeChanged(bool dataMode);
			void				sendFilter(const QString & generatedFilter, const QString & filter);
			
			

		

public slots:
			void				refresh();
			void				runFilter();
			void				handleColumnChanged(		const Column * column);
			void				handleLabelsReordered(		const Column * column);
			void				handleColumnTypeChanged(	const Column * column);
			void				handleColumnsAboutToBeRemoved(const QModelIndex &parent, int first, int last) { emit columnsBeingRemoved(first, (last-first)+1); }

			bool				setColumnTypes(intset columnIndexes, columnType newColumnType);
			

public:
		Filter *	createFilter(const std::string & name, bool createIfMissing = true) { return new Filter(this, name, createIfMissing); }
			
private:
			void					upgradeTo019(const Json::Value & emptyVals);
			void					upgrade019To095();
			void					setEmptyValuesJsonOldStuff(	const Json::Value & emptyValues);

			
private slots:
			void				handleDataSetChanged(	QStringList				changedColumns,
														QStringList				missingColumns,
														QMap<QString, QString>	changeNameColumns,
														bool					rowCountChanged,
														bool					hasNewColumns);

public:
	static QVariant				getDataSetViewLines(bool up, bool left, bool down, bool right)									;
			
	
protected:
	bool						getRowFilter(int row)																			const;
	bool						getColumnInDragNDropShownFilter(int columnIndex)												const;
	bool						getColumnInDragNDropShownFilter(Column * column)												const;
		
private:
	Columns						_columns;
	Filter					*	_filter					= nullptr;
	EmptyValues				*	_emptyValues			= nullptr;
	int							_dataSetID				= -1,
								_rowCount				= -1,
								_writeBatchedToDBDepth	= 0;
	ColumnSet					_changedDuringBatch		= {};
	long						_dataFileTimestamp		= 0;
	std::string					_dataFilePath,
								_databaseJson;
	
	bool						_dataFileSynch			= false,
								_dataMode				= false;
	static stringset			_defaultEmptyvalues;	// Default empty values if workspace do not have its own empty values (used for backward compatibility)
	std::string					_description;
};

#endif // DATASET_H
