//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef FILEPACKAGE_H
#define FILEPACKAGE_H

#include <QAbstractItemModel>
#include "common.h"
#include "dataset.h"
#include "version.h"
#include <map>
#include "boost/signals2.hpp"
#include "jsonredirect.h"
#include "computedcolumns.h"
#include "enumutilities.h"

#define DEFAULT_FILTER		"# Add filters using R syntax here, see question mark for help.\n\ngeneratedFilter # by default: pass the non-R filter(s)"
#define DEFAULT_FILTER_JSON	"{\"formulas\":[]}"
#define DEFAULT_FILTER_GEN	"generatedFilter <- rep(TRUE, rowcount)"

DECLARE_ENUM(parIdxType, data, label, filter, root)

class DataSetPackage : public QAbstractItemModel //Not QAbstractTableModel because of: https://stackoverflow.com/a/38999940
{
	Q_OBJECT
	Q_PROPERTY(int columnsFilteredCount READ columnsFilteredCount NOTIFY columnsFilteredCountChanged)

	typedef std::map<std::string, std::map<int, std::string>> emptyValsType;

public:
	enum class	specialRoles { filter = Qt::UserRole, lines, maxColString, columnIsComputed, computedColumnIsInvalidated, columnIsFiltered, computedColumnError, value, columnType };

									DataSetPackage(QObject * parent);
				void				reset();

		QHash<int, QByteArray>		roleNames()																			const	override;
				int					rowCount(	const QModelIndex &parent = QModelIndex())								const	override;
				int					columnCount(const QModelIndex &parent = QModelIndex())								const	override;
				QVariant			data(		const QModelIndex &index, int role = Qt::DisplayRole)					const	override;
				QVariant			headerData(	int section, Qt::Orientation orientation, int role = Qt::DisplayRole )	const	override;
				bool				setData(	const QModelIndex &index, const QVariant &value, int role)						override;
				Qt::ItemFlags		flags(		const QModelIndex &index)												const	override;
				bool				hasChildren(const QModelIndex &index)												const	override;
				parIdxType			parentModelIndexIs(const QModelIndex &index)										const;
				QModelIndex			parentModelForType(parIdxType type, int column = 0)									const;

				void				storeInEmptyValues(std::string columnName, std::map<int, std::string> emptyValues)	{ _emptyValuesMap[columnName] = emptyValues;	}
				void				resetEmptyValues()																	{ _emptyValuesMap.clear();											}

				std::string			id()								const	{ return _id;							}
				bool				isReady()							const	{ return _analysesHTMLReady;			}
				DataSet			*	dataSet()									{ return _dataSet;						}
				bool				isLoaded()							const	{ return _isLoaded;						 }
				bool				isArchive()							const	{ return _isArchive;					  }
				bool				isModified()						const	{ return _isModified;					   }
				std::string			dataFilter()						const	{ return _dataFilter;						}
				std::string			initialMD5()						const	{ return _initialMD5;						 }
				bool				hasAnalyses()						const	{ return _analysesData.size() > 0;			  }
				std::string			dataFilePath()						const	{ return _dataFilePath;						   }
		const	std::string		&	analysesHTML()						const	{ return _analysesHTML;							}
		const	Json::Value		&	analysesData()						const	{ return _analysesData;							 }
		const	std::string		&	warningMessage()					const	{ return _warningMessage;						  }
		const	Version			&	archiveVersion()					const	{ return _archiveVersion;						   }
		const	emptyValsType	&	emptyValuesMap()					const	{ return _emptyValuesMap;   						}
				bool				dataFileReadOnly()					const	{ return _dataFileReadOnly;						     }
				uint				dataFileTimestamp()					const	{ return _dataFileTimestamp;					      }
		const	Version			&	dataArchiveVersion()				const	{ return _dataArchiveVersion;						   }
		const	std::string		&	filterConstructorJson()				const	{ return _filterConstructorJSON;					    }

				void				setDataArchiveVersion(Version archiveVersion)	{ _dataArchiveVersion			= archiveVersion;	}
				void				setFilterConstructorJson(std::string json)		{ _filterConstructorJSON		= json;				}
				void				setAnalysesData(Json::Value analysesData)		{ _analysesData					= analysesData;		}
				void				setArchiveVersion(Version archiveVersion)		{ _archiveVersion				= archiveVersion;	}
				void				setWarningMessage(std::string message)			{ _warningMessage				= message;			}
				void				setDataFilePath(std::string filePath)			{ _dataFilePath					= filePath;			}
				void				setInitialMD5(std::string initialMD5)			{ _initialMD5					= initialMD5;		}
				void				setDataFileTimestamp(uint timestamp)			{ _dataFileTimestamp			= timestamp;		}
				void				setDataFileReadOnly(bool readOnly)				{ _dataFileReadOnly				= readOnly;			}
				void				setAnalysesHTML(std::string html)				{ _analysesHTML					= html;				}
				void				setDataFilter(std::string filter)				{ _dataFilter					= filter;			}
				void				setDataSet(DataSet * dataSet);
				void				setIsArchive(bool isArchive)					{ _isArchive					= isArchive;		}
				void				setModified(bool value);
				void				setAnalysesHTMLReady()							{ _analysesHTMLReady			= true;				}
				void				setId(std::string id)							{ _id							= id;				}
				void				setWaitingForReady()							{ _analysesHTMLReady			= false;			}
				void				setLoaded()										{ _isLoaded						= true;				}
				void				setHasAnalysesWithoutData()						{ _hasAnalysesWithoutData		= true;				}

				bool				isColumnNameFree(std::string name)		const;
				bool				isColumnComputed(size_t colIndex)		const;
				bool				isColumnComputed(std::string name)		const;
				bool				isColumnInvalidated(size_t colIndex)	const;
				std::string			getComputedColumnError(size_t colIndex) const;

				void				removeColumn(std::string name)		{ _computedColumns.removeComputedColumn(name);	}
				void				informComputedColumnsOfPackage()	{ _computedColumns.setPackage(this); }

				ComputedColumns	*	computedColumnsPointer();

				void				pauseEngines()			{ emit pauseEnginesSignal();				}
				void				resumeEngines()			{ emit resumeEnginesSignal();				}
				bool				enginesInitializing()	{ return emit enginesInitializingSignal();	}

	Q_INVOKABLE bool				isColumnNameFree(QString name)						{ return isColumnNameFree(name.toStdString()); }
	Q_INVOKABLE bool				getRowFilter(int row)					const;
	Q_INVOKABLE	QVariant			columnTitle(int column)					const;
	Q_INVOKABLE QVariant			columnIcon(int column)					const;
	Q_INVOKABLE QVariant			getColumnTypesWithCorrespondingIcon()	const;
	Q_INVOKABLE bool				columnHasFilter(int column)				const;
	Q_INVOKABLE bool				columnUsedInEasyFilter(int column)		const;
	Q_INVOKABLE void				resetAllFilters();
	Q_INVOKABLE int					setColumnTypeFromQML(int columnIndex, int newColumnType);

				size_t				addColumnToDataSet();
				int					columnsFilteredCount();

				bool				setColumnType(int columnIndex, Column::ColumnType newColumnType);
				Column::ColumnType	getColumnType(int columnIndex);
				bool				isComputedColumn(int colIndex)				const { return isColumnComputed(colIndex); }
				bool				isComputedColumnInvalided(int colIndex)		const { return isColumnInvalidated(colIndex); }

				void				beginLoadingData();
				void				endLoadingData();
				void				beginSynchingData();
				void				endSynchingData(std::vector<std::string>			&	changedColumns,
													std::vector<std::string>			&	missingColumns,
													std::map<std::string, std::string>	&	changeNameColumns,
													bool									rowCountChanged,
													bool									hasNewColumns);

signals:
				void				dataSynched(	std::vector<std::string>			&	changedColumns,
													std::vector<std::string>			&	missingColumns,
													std::map<std::string, std::string>	&	changeNameColumns,
													bool									rowCountChanged,
													bool									hasNewColumns);

				void				columnsFilteredCountChanged();
				void				badDataEntered(const QModelIndex index);
				void				allFiltersReset();
				void				dataSetChanged(DataSet * newDataSet);
				void				columnDataTypeChanged(std::string columnName);
				void				isModifiedChanged(DataSetPackage *source);
				void				pauseEnginesSignal();
				void				resumeEnginesSignal();
				bool				enginesInitializingSignal();

public slots:
				void				refresh() { beginResetModel(); endResetModel(); }
				void				refreshColumn(Column * column);
				void				columnWasOverwritten(std::string columnName, std::string possibleError);
				void				notifyColumnFilterStatusChanged(int columnIndex);
				void				setColumnsUsedInEasyFilter(std::set<std::string> usedColumns);


private:

	DataSet					*	_dataSet					= nullptr;
	emptyValsType				_emptyValuesMap;

	std::string					_analysesHTML,
								_id,
								_warningMessage,
								_initialMD5,
								_dataFilePath,
								_dataFilter					= DEFAULT_FILTER,
								_filterConstructorJSON		= DEFAULT_FILTER_JSON;

	bool						_isArchive					= false,
								_dataFileReadOnly,
								_isModified					= false,
								_isLoaded					= false,
								_hasAnalysesWithoutData		= false,
								_analysesHTMLReady			= false,
								_enginesLoadedAtBeginSync;

	Json::Value					_analysesData;
	Version						_archiveVersion,
								_dataArchiveVersion;

	uint						_dataFileTimestamp;

	ComputedColumns				_computedColumns;
	bool						_synchingData;
	std::map<std::string, bool> _columnNameUsedInEasyFilter;
};

#endif // FILEPACKAGE_H
