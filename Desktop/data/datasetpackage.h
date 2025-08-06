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

#include <map>
#include <QUrl>
#include <QTimer>
#include <cstddef>
#include "version.h"
#include <QFileInfo>
#include <json/json.h>
#include "undostack.h"
#include "dataset.h"
#include "databaseinterface.h"
#include <QSortFilterProxyModel>

class EngineSync;

///
/// DataSetPackage is a utility class that should probably have been called Workspace
///
/// It handles loading and creation of DataSet(Q) which handles interaction with the database via itself and the other DataSetBaseNodes
/// 
class DataSetPackage : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int			columnsFilteredCount	READ columnsFilteredCount										NOTIFY columnsFilteredCountChanged	)
	Q_PROPERTY(QString		folder					READ folder						WRITE setFolder					NOTIFY folderChanged				)
	Q_PROPERTY(QString		windowTitle				READ windowTitle												NOTIFY windowTitleChanged			)
	Q_PROPERTY(bool			modified				READ isModified					WRITE setModified				NOTIFY isModifiedChanged			)
	Q_PROPERTY(bool			modifiedAfterAutoSave	READ isModifiedAfterAutoSave	WRITE setModifiedAfterAutoSave	NOTIFY isModifiedAfterAutoSaveChanged			)
	Q_PROPERTY(bool			loaded					READ isLoaded					WRITE setLoaded					NOTIFY loadedChanged				)
	Q_PROPERTY(QString		currentFile				READ currentFile				WRITE setCurrentFile			NOTIFY currentFileChanged			)
	Q_PROPERTY(bool			dataMode				READ dataMode													NOTIFY dataModeChanged				)
	Q_PROPERTY(bool			synchingExternally		READ synchingExternally			WRITE setSynchingExternally		NOTIFY synchingExternallyChanged	) //might have to be moved to dataset when we have multiple datasets, alse CurrentDataFile in FileMenu will need to be looked at...
	Q_PROPERTY(bool			manualEdits				READ manualEdits				WRITE setManualEdits			NOTIFY manualEditsChanged			) ///< Did the user change something in the data in such a way that external synching should be disabled if enabled?
	Q_PROPERTY(DataSet *	dataSet					READ dataSet											NOTIFY DataSetChanged				) // Sorry about the capitalization
	
public:
	
	static DataSetPackage *	pkg() { return _singleton; }

							DataSetPackage(QObject * parent);
							~DataSetPackage();
		static Filter	*	filter();
		DataSet			*	dataSet() { return _dataSet; }
		void				setEngineSync(EngineSync * engineSync);
		void				reset(bool newDataSet = true);
		void				setDataSetSize(size_t columnCount, size_t rowCount);
		void				setDataSetRowCount(size_t rowCount)					{ setDataSetSize(dataColumnCount(),		rowCount); }
		void				increaseDataSetColCount(size_t rowCount)			{ setDataSetSize(dataColumnCount() + 1,	rowCount); }

		void				createDataSet();	///< Creates *OR* recreates a dataset in database
		void				connectDataSet();
        void                loadDataSet(std::function<void(float)> progressCallback = [](float){});      ///< Assumes internal.sqlite has just been loaded from a JASPFile and will init DataSet etc with it.
		void				deleteDataSet();	///< Deletes dataset from memory but not from database
		bool				hasDataSet() { return _dataSet; }

		void				pauseEngines();
		void				resumeEngines();
		void				enginesPrepareForData();
		void				enginesReceiveNewData();
		bool				enginesInitializing()	{ return emit enginesInitializingSignal();	}

		void				waitForExportResultsReady();

		void				beginLoadingData(	bool informEngines = true);
		void				stopEngines();
		void				restartEngines();
		void				endLoadingData(		bool informEngines = true);
		void				beginSynchingData(	bool informEngines = true);
		void				endSynchingDataChangedColumns(stringvec	&	changedColumns,		bool hasNewColumns = false, bool informEngines = true);
		void				endSynchingData(const stringvec		&	changedColumns,
											const stringvec		&	missingColumns,
											const strstrmap		&	changeNameColumns,  //origname -> newname
											bool										rowCountChanged,
											bool										hasNewColumns,		bool informEngines = true);

				QString				insertColumnSpecial(int column, const QMap<QString, QVariant>& props, bool setManualEdits = true);
				QString				appendColumnSpecial(			const QMap<QString, QVariant>& props, bool setManualEdits = true);

				int					dataRowCount()		const;
				int					dataColumnCount()	const;
				void				refresh();


				std::string			id()								const	{ return _id;							}
				QString				name()								const;
				QString				folder()							const	{ return _folder;						}
				bool				dataMode()							const;
				
				
				bool				isReady()							const	{ return _analysesHTMLReady;			}
				bool				isLoaded()							const	{ return _isLoaded;						 }
				bool				isJaspFile()						const	{ return _isJaspFile;					  } ///< for readability
				bool				isModified()						const	{ return _isModified;					   }
				bool				isModifiedAfterAutoSave()			const	{ return _isModifiedAfterAutoSave;	 	    }
				bool				hasAnalysesWithoutData()			const	{ return _hasAnalysesWithoutData;			 }
				std::string			initialMD5()						const	{ return _initialMD5;						 }
				bool				manualEdits()						const;
				QString				windowTitle()						const;
				QString				description()						const;
				QString				currentFile()						const	{ return _currentFile;						 }
				QString				autoSavedFileName()					const;
				bool				hasAnalyses()						const	{ return _analysesData.size() > 0;				}
				bool				synchingData()						const	{ return _synchingData;								}
				std::string			dataFilePath()						const	{ return _dataSet ? _dataSet->dataFilePath() : "";  }
				bool				dataFileCanHaveLabels()				const;
				bool				isDatabase()						const	{ return _database != Json::nullValue;				}
		const	Json::Value		&	databaseJson()						const	{ return _database;								}
		const	QString			&	analysesHTML()						const	{ return _analysesHTML;							}
		const	Json::Value		&	analysesData()						const	{ return _analysesData;							}
		const	std::string		&	warningMessage()					const	{ return _warningMessage;						}
		const	Version			&	archiveVersion()					const	{ return _archiveVersion;						}
		const	Version			&	jaspVersion()						const	{ return _jaspVersion;							}

				// The data file might be read-only if it comes from the examples or read from an external database
				bool				dataFileReadOnly()					const	{ return _dataFileReadOnly;						}
				bool				currentJaspFileIsNonSaveable()		const;
				bool				filePathIsNonSaveable(const QString &path) const;
				long				dataFileTimestamp()					const	{ return _dataSet ? _dataSet->dataFileTimestamp() : 0;	}
				bool				isDatabaseSynching()				const	{ return _databaseIntervalSyncher.isActive();	}
				bool				filterShouldRunInit()				const	{ return _filterShouldRunInit && isLoaded();					}


				void				setFilterShouldRunInit(bool shouldIt)				{ _filterShouldRunInit			= shouldIt;			}
				void				setAnalysesData(const Json::Value & analysesData);
				void				setArchiveVersion(Version archiveVersion)			{ _archiveVersion				= archiveVersion;	}
				void				setJaspVersion(Version jaspVersion)					{ _jaspVersion					= jaspVersion;		}
				void				updateDbToCurrentVersion();							///< Should be ran immediately after loading the jasp file
				void				setWarningMessage(std::string message)				{ _warningMessage				= message;			}
				void				setDataFilePath(std::string filePath, long timestamp = 0);
				void				setDatabaseJson(const Json::Value & dbInfo);
				void				setInitialMD5(std::string initialMD5)				{ _initialMD5					= initialMD5;		}
				void				setDataFileReadOnly(bool readOnly)					{ _dataFileReadOnly				= readOnly;			}
				void				setAnalysesHTML(const QString & html)				{ _analysesHTML					= html;				}
				void				setIsJaspFile(bool isJaspFile)						{ _isJaspFile					= isJaspFile;		}
				void				setHasAnalysesWithoutData()							{ _hasAnalysesWithoutData		= true;				}
				void				setModifiedAfterAutoSave(bool value);
				void				setModified(bool value = true);
				void				enableModified() { setModified(); }
				void				setAnalysesHTMLReady()								{ _analysesHTMLReady			= true;				}
				void				setId(std::string id)								{ _id							= id;				}
				void				setWaitingForReady()								{ _analysesHTMLReady			= false;			}
				void				setManualEdits(bool newManualEdits = true);
				void				enableManualEdits() { setManualEdits(); }
				void				setLoaded(bool loaded = true);
				void				setDescription(const QString& description);
				
				void						initializeComputedColumns();
				
				void						pasteSpreadsheet(size_t row, size_t column, const std::vector<std::vector<QString>> & values, const std::vector<std::vector<QString>> & labels, const intvec & colTypes, const QStringList & colNames, const std::vector<boolvec> & selected = {}); ///< If selected.size() >0 it is assumed to be the same size as labels/values. And it will make sure that it will only overwrite values where it is `true`

				void						columnSetDefaultValues(	const std::string	& columnName, columnType colType = columnType::unknown, bool emitSignals = true);
				Column *					createColumn(			const std::string	& name,		columnType colType);
				Column *					createComputedColumn(	const std::string	& name,		columnType type, computedColumnType desiredType, Analysis * analysis = nullptr);
				void						renameColumn(			const std::string	& oldColumnName, const std::string & newColumnName);
				void						removeColumn(			const std::string	& name);
				bool						columnExists(			Column				* column);
				void						columnsReorder(			const stringvec		& order);
				
				stringvec					getColumnNames();
		std::map<std::string, columnType>	getColumnTypesMap();
				bool						isColumnDifferentFromStringLookUps(const std::string & columnName, const std::string & title, size_t rows,	const std::function<std::string(size_t)> valueLookup, const std::function<std::string(size_t)> labelLookup, const stringset & strEmptyVals);
				int							findIndexByName(const std::string & name)	const;

				QVariant					getColumnTypesWithIcons()										const;
				std::string					getComputedColumnError(		size_t					colIndex)	const;

				bool						isColumnUsedInEasyFilter(	const std::string	&	name)		const;
				bool						isColumnNameFree(			const std::string	&	name)		const;
				bool						isColumnNameFree(			const QString		&	name)		const	{ return isColumnNameFree(name.toStdString()); }
				bool						isColumnComputed(			size_t					colIndex)	const;
				bool						isColumnComputed(			const std::string	&	name)		const;
				bool						isColumnAnalysisNotComputed(const std::string	&	name)		const;

				bool						isColumnInvalidated(		size_t					colIndex)	const;


				bool						setColumnTypes(intset	columnIndexes,	columnType newColumnType);
				

				int							columnsFilteredCount();

				void						writeDataSetToOStream(std::ostream & out, bool includeComputed);

				int							getColumnIndex(						const std::string & name)			const	{ return !_dataSet ? -1 : _dataSet->getColumnIndex(name); }
				int							getColumnIndex(						const QString	  & name)			const	{ return getColumnIndex(name.toStdString()); }
				Column*						getColumn(							const std::string & name)					{ return _dataSet->column(name); }
				enum columnType				getColumnType(						size_t				columnIndex)	const;
				enum columnType				getColumnType(						const QString	  &	name)			const;
				std::string					getColumnName(						size_t				columnIndex)	const;
				stringvec					getColumnDataStrs(					size_t				columnIndex);
				void						setColumnName(						size_t				columnIndex, const std::string	& newName);
				void						setColumnTitle(						size_t				columnIndex, const std::string	& newTitle);
				void						setColumnDropLevels(				size_t					columnIndex, dropLevelsType dropLevels);
				void						setColumnDescription(				size_t				columnIndex, const std::string	& newDescription);
				void						setColumnComputedType(				size_t				columnIndex, computedColumnType	type);
				void						setColumnComputedType(				const std::string &	columnName,	computedColumnType	type);
				void						setColumnComputeFilter(				size_t columnIndex, const std::string &newFilter);
				void						setColumnHasCustomEmptyValues(		size_t				columnIndex, bool				  hasCustomEmptyValue);
				void						setColumnCustomEmptyValues(			size_t				columnIndex, const stringset	& customEmptyValues);
				void						columnsReverseValues(				intset				columnIndex);
				void						columnsSetAutoSortForColumns(		std::map<int,bool>	columnutoSort);
				qsizetype					getMaximumColumnWidthInCharacters(	int					columnIndex)				const;
				Json::Value					serializeColumn(					const std::string & columnName)					const;
				void						deserializeColumn(					const std::string & columnName, const Json::Value& col);

				
				bool						labelNeedsFilter(					size_t				columnIndex)				const;
				void						labelMoveRows(						size_t				columnIndex, std::vector<qsizetype> rows, bool up);
				void						labelReverse(						size_t				columnIndex);
				void						resetAllFilters();
				std::vector<bool>			filterVector();
				void						setFilterVectorWithoutModelUpdate(std::vector<bool> newFilterVector) { if(_dataSet) _dataSet->shownFilter()->setFilterVector(newFilterVector); }
				
	static		int							thresholdScale();
	static		int							orderByValueByDefault();
				const stringset&			workspaceEmptyValues()										const;
				void						setWorkspaceEmptyValues(const stringset& emptyValues, bool resetModel = true);
				void						setDefaultWorkspaceEmptyValues();

				void						databaseStartSynching(bool syncImmediately);
				void						databaseStopSynching();
				bool						synchingExternally() const;
				void						checkComputedColumnDependenciesForAnalysis(	Analysis * analysis);
				stringset					columnsCreatedByAnalysis(					Analysis * analysis);
				std::string					freeNewColumnName(size_t startHere);

				void						dbDelete();
				void						emitColumnChanged(const QString &colName); //temporary until ColumnQ exists
				
				void						resetVariableTypes();

signals:
				void				datasetChanged(	QStringList				changedColumns,
													QStringList				missingColumns,
													QMap<QString, QString>	changeNameColumns,
													bool					rowCountChanged,
													bool					hasNewColumns);
				void				columnsFilteredCountChanged();
				void				runFilter();
				void				badDataEntered(const QModelIndex index);
				void				allFiltersReset();
				void				labelFilterChanged();
				void				labelChanged(			QString columnName, QString originalLabel, QString newLabel);
				void				columnDataTypeChanged(	QString columnName);
				void				labelsReordered(		QString columnName);
				void				columnAddedManually(	QString columnName);
				void				chooseColumn(			int		colId);
				void				isModifiedChanged();
				void				isModifiedAfterAutoSaveChanged();
				void				enginesPrepareForDataSignal();
				void				enginesReceiveNewDataSignal();
				bool				enginesInitializingSignal();
				void				filteredOutChanged(int column);
				bool				checkDoSync();
				void				modelInit();
				void				nameChanged();
				void				folderChanged();
				void				windowTitleChanged();
				void				loadedChanged();
				void				currentFileChanged();
				void				synchingIntervalPassed();
				void				newDataLoaded();
				void				dataModeChanged(bool dataMode);
				void				synchingExternallyChanged(bool);
				bool				askUserForExternalDataFile();
				void				checkForDependentColumnsToBeSent(	QString columnName);
				void				showWarning(						QString title, QString msg);
				void				manualEditsChanged();
				void				columnsBeingRemoved(				int columnIndex, int count);
				void				workspaceEmptyValuesChanged();
				void				descriptionChanged();
				void				refreshAllAnalyses();
				void				refreshAllCompCols();
				void				makeAnAutoSave();
				void				DataSetChanged();
				void				sendFilter(const QString & generatedFilter, const QString & filter);

public slots:
				void				refreshColumn(						QString columnName);
				void				columnWasOverwritten(				const std::string & columnName, const std::string & possibleError);
				void				setCurrentFile(						QString currentFile);
				void				setFolder(							QString folder);
				void				generateEmptyData();
				void				onDataModeChanged(					bool dataMode);
				void				setSynchingExternallyFriendly(		bool synchingExternally);
				void				setSynchingExternally(				bool synchingExternally);
				Column			 *	requestComputedColumnCreation(		const std::string & columnName, Analysis * analysis);
				bool				requestColumnCreation(				const std::string & columnName, Analysis * analysis, columnType type);
				bool				requestComputedColumnDestruction(	const std::string & columnName, Analysis * analysis);
				void				checkDataSetForUpdates();
				void				doWalCheckPoint();
				void				handleAutoSave();

				void				prepareForLanguageChange();
				void				languageChangeDone();
				void				handleAutoSavePrefChange();
				
private:
				bool				isThisTheSameThreadAsEngineSync();
				void				columnsApply(intset columnIndexes, std::function<bool (Column *)>		applyThis);
				void				columnsApply(intset columnIndexes, std::function<bool (Column *, int)>	applyThis);

private:
	static DataSetPackage	*	_singleton;
	DatabaseInterface		*	_db							= nullptr;
	DataSet					*	_dataSet					= nullptr;
	EngineSync				*	_engineSync					= nullptr;

	QString						_currentFile,
								_folder,
								_analysesHTML;
	std::string					_id,
								_warningMessage,
								_initialMD5;

	bool						_isJaspFile					= false,
								_dataFileReadOnly,
								_isModified					= false,
								_isModifiedAfterAutoSave	= false,
								_isLoaded					= false,
								_hasAnalysesWithoutData		= false,
								_analysesHTMLReady			= false,
								_filterShouldRunInit		= false,
								_manualEdits				= false,
								_waitingForLanguageChange	= false;

	Json::Value					_analysesData,
								_database					= Json::nullValue;
	Version						_archiveVersion,
								_jaspVersion;

	bool						_synchingData				= false;

	QTimer						_databaseIntervalSyncher,
								_doWalCheckPointTimer,
								_autoSaveTimer;
	UndoStack				*	_undoStack					= nullptr;
};

#endif // FILEPACKAGE_H
