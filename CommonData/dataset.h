//
// Copyright (C) 2013-2026 University of Amsterdam
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
#ifndef DATASET_H
#define DATASET_H

#include "datasetbasenode.h"
#include "column.h"
#include "filter.h"
#include "emptyvalues.h"
#include "version.h"

class DataSet;
class Workspace;

	using DataSets = std::vector<DataSet*>;

class DataSet : public DataSetBaseNode
{
	Q_OBJECT
public:
	typedef 	std::map<std::string,columnType> colTypeMap;
	
							DataSet(int index = -1); ///< index==-1: create a new dataSet, >0: load that dataSet, 0: do nothing
							~DataSet();
	
			Filter		*	filter()						{ return	_filter;	}
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
			bool			showRSyntax()			const { return _showRSyntax;			}
			bool			isComputed()			const { return _isComputed;			}
	const	std::string &	dataFilePath()			const { return _dataFilePath;			}
			int				dataFileTimestamp()		const { return _dataFileTimestamp;		}
	const	std::string &	databaseJson()			const { return _databaseJson;			}
	const	std::string &	computeCode()			const { return _computeCode;			}
	const	std::string &	dependsOn()				const { return _dependsOn;				}
			bool			writeBatchedToDB()		const { return _writeBatchedToDBDepth;		}
			void			batchColumnHadChange(Column *col);

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
			Column		*	createComputedColumn(const std::string & name, columnType type, computedColumnType desiredType, int analysisId = -1);
			int				getColumnIndex(	const	std::string &	name	) const;
			void			resetFilterCounters();
			void			runComputedColumn(const std::string & name, const std::string & code, columnType type);
			void			resetAllFilters();
			bool			synchingData() const { return false; }
			bool			dataFileCanHaveLabels() const { return false; }
			int				columnsLabelFilteredCount() const { return 0; }
			void			columnRefreshed(Column *) {}
			static QVariant	getDataSetViewLines(bool, bool, bool, bool) { return QVariant(); }
			bool			isColumnNameFree(const std::string &) { return true; }

			const Columns	&	columnsRef() const { return _columns; }
			int				columnIndex(	const	Column		*	col		) const;
			void			columnsReorder(			stringvec		order	); ///< Expects a sane order vector, with or without computed columns

			bool			allColumnsPassFilter()					const;

			size_t			getMaximumColumnWidthInCharacters(size_t columnIndex) const;
			stringvec		getColumnNames();
			colTypeMap		getColumnTypesMap();

			void			setDataFile( const std::string & dataFilePath, long timestamp)	{ _dataFilePath	= dataFilePath;	_dataFileTimestamp = timestamp; dbUpdate(); }
			void			setDatabaseJson(	const std::string & databaseJson)	{ _databaseJson		= databaseJson;			dbUpdate(); }
			void			setDataFileSynch(	bool synchronizing)					{ _dataFileSynch	= synchronizing;		dbUpdate(); }
			void			setShowRSyntax(		bool showRSyntax)					{ _showRSyntax		= showRSyntax;			dbUpdate(); }
			void			setIsComputed(		bool isComputed)					{ _isComputed		= isComputed;			dbUpdate(); }
			void			setComputeCode(		const std::string & code)			{ _computeCode		= code;				dbUpdate(); }
			void			setDependsOn(		const std::string & deps)			{ _dependsOn		= deps;				dbUpdate(); }
			char			csvDelimiter()		const								{ return _csvDelimiter; }
			void			setCsvDelimiter(	char delimiter)						{ _csvDelimiter		= delimiter;			dbUpdate(); }

			void			setColumnCount(	size_t colCount);
			void			setRowCount(	size_t rowCount, bool alsoLoadData = true);

			void			incRevision() override;
			bool			checkForUpdates(stringvec * colsChanged = nullptr, stringvec * colsRemoved = nullptr, bool * newColumns = nullptr, bool * rowCountChanged = nullptr);

	const	Columns			&	computedColumns() const;
			
			void			refresh(bool doDataChanged = true);
			
			void			loadOldComputedColumnsJson(const Json::Value & json); ///< Should act the same as the old ComputedColumns::fromJson() to allow loading "older jaspfiles"
			stringset		findUsedColumnNames(std::string searchThis);

			DatabaseInterface	 &	db();
	const	DatabaseInterface	 &	db() const;
			Workspace			 *	workspace() const;
	
			DataSetBaseNode		 *	dataNode()		const { return _dataNode; }
			DataSetBaseNode		 *	filtersNode()	const { return _filtersNode; }

			void					setEmptyValuesJson(			const Json::Value & emptyValues, bool updateDB = true);
			
	const	stringset			&	workspaceEmptyValues()															const	{ return _emptyValues->emptyStrings();								}
			void					setWorkspaceEmptyValues(	const stringset& values);
	const	std::string			&	description()																	const	{ return _description; }
			void					setDescription(				const std::string& desc);
			Json::Value				jsonForCompare() const;
			
private:
			void					upgradeEmptyValsFrom018To019(const Json::Value & emptyVals);
			void					setEmptyValuesJsonOldStuff(	const Json::Value & emptyValues);
			
			
private:
	DataSetBaseNode			*	_dataNode				= nullptr, //To make sure we have a pointer to flesh out the node hierarchy we add a "data" node, so we can place it next to the "filters" node in the tree
							*	_filtersNode			= nullptr;
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
								_showRSyntax			= false,
								_isComputed				= false;
	std::string					_computeCode,
								_dependsOn;
	char						_csvDelimiter			= '\0';
	static stringset			_defaultEmptyvalues;	// Default empty values if workspace do not have its own empty values (used for backward compatibility)
	std::string					_description;
};

#endif // DATASET_H
