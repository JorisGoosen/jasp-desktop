#ifndef DATASET_H
#define DATASET_H

#include "datasetbasenode.h"
#include "column.h"
#include "filter.h"
#include "emptyvalues.h"

class DataSet : public DataSetBaseNode
{
public:
							DataSet(int index = -1); ///< index==-1: create a new dataSet, >0: load that dataSet, 0: do nothing
							~DataSet();
	
			Filter		*	shownFilter()		const 		{ return	_filter;	}
			Columns		&	columns()			const		{ return	const_cast<Columns&>(_columns);	}
    const	EmptyValues *	emptyValues()       const		{ return	_emptyValues; }
			EmptyValues *	emptyValues()					{ return	_emptyValues; }

			Column		*	column(		const std::string & name);
			Column		*	column(		size_t				columnIndex);

			Column		*	operator[](	size_t				columnIndex)	{ return column(columnIndex); }
			Column		*	operator[](	const std::string &	columnName)		{ return column(columnName); }
	
			int				id()					const { return _dataSetID;				}
			int				columnCount()			const ;
			int				rowCount()				const ;
			bool			dataFileSynch()			const { return _dataFileSynch;			}
	const	std::string &	dataFilePath()			const { return _dataFilePath;			}
			int				dataFileTimestamp()		const { return _dataFileTimestamp;		}
	const	std::string &	databaseJson()			const { return _databaseJson;			}
			bool			writeBatchedToDB()		const { return _writeBatchedToDB;		}

			int				columnsFilteredCount()	const;

			void			dbCreate();
			void			dbUpdate();
			void			dbLoad(int index = -1, std::function<void(float)> progressCallback = [](float){}, bool do019Fix = false);
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
			void			columnsReorder(	const	stringvec	&	order	); ///< Expects a sane order vector

			bool			allColumnsPassFilter()					const;

			qsizetype		getMaximumColumnWidthInCharacters(size_t columnIndex) const;
			stringvec		getColumnNames();

			void			setDataFile(		const std::string & dataFilePath);
			void			setDataTimestamp(	long timestamp);
			void			setDatabaseJson(	const std::string & databaseJson);
			void			setDataFileSynch(	bool synchronizing);

			void			setColumnCount(	size_t colCount);
			void			setRowCount(	size_t rowCount);

			void			incRevision() override;
			bool			checkForUpdates(stringvec * colsChanged = nullptr, stringvec * colsRemoved = nullptr, bool * newColumns = nullptr, bool * rowCountChanged = nullptr);

			const Columns &	computedColumns() const;
			
			void			loadOldComputedColumnsJson(const Json::Value & json); ///< Should act the same as the old ComputedColumns::fromJson() to allow loading "older jaspfiles"
			stringset		findUsedColumnNames(std::string searchThis);
			bool			initColumnWithStrings(int colIndex, const std::string & newName, const stringvec &values, const stringvec & labels, const std::string & title, columnType desiredType, const stringset & emptyValues, int threshold, bool orderLabelsByValue);

			DatabaseInterface	 &	db();
	const	DatabaseInterface	 &	db() const;
	
			void					setEmptyValuesJson(			const Json::Value & emptyValues, bool updateDB = true);
			
	const	stringset			&	workspaceEmptyValues()															const	{ return _emptyValues->emptyStrings();								}
	const	std::string			&	description()																	const	{ return _description; }
			void					setWorkspaceEmptyValues(	const stringset& values);
			void					setDescription(				const std::string& desc);

public:
		Filter *	createFilter(const std::string & name, bool createIfMissing = true) { return _createFilter(name, createIfMissing); }

protected:
		virtual Column *	_createColumn(int id=-1);
		virtual Filter *	_createFilter();
		virtual Filter *	_createFilter(const std::string & name, bool createIfMissing = true);
			
private:			
			void					upgradeTo019(const Json::Value & emptyVals);
			void					setEmptyValuesJsonOldStuff(	const Json::Value & emptyValues);

protected:
	std::function<void()>		_descriptionChanged;
	std::function<void()>		_dataFilePathChanged;
	std::function<void()>		_dataTimestampChanged;
	std::function<void()>		_dataSynchChanged;
	std::function<void()>		_databaseJsonChanged;
			
private:
	Columns						_columns;
	Filter					*	_filter					= nullptr;
	EmptyValues				*	_emptyValues			= nullptr;
	int							_dataSetID				= -1,
								_rowCount				= -1;
	long						_dataFileTimestamp		= 0;
	std::string					_dataFilePath,
								_databaseJson;
	
	bool						_writeBatchedToDB		= false,
								_dataFileSynch			= false;
	static stringset			_defaultEmptyvalues;	// Default empty values if workspace do not have its own empty values (used for backward compatibility)
	std::string					_description;
};

#endif // DATASET_H
