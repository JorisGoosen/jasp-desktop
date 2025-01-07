#ifndef DATASETQ_H
#define DATASETQ_H

#include <dataset.h>
#include <QAbstractTableModel>

class ColumnQ;
class FilterQ;
class DataSetQ : public DataSet, public QAbstractTableModel
{
	Q_OBJECT
	
	//Would be nice to have EmptyValuesQ also and make it available as a property here
	Q_PROPERTY(QString	description				READ descriptionQ			WRITE setDescriptionQ		NOTIFY descriptionChanged			)
	Q_PROPERTY(QString	dataFile				READ dataFileQ				WRITE setdataFileQ			NOTIFY dataFileChanged				)
	Q_PROPERTY(QString	databaseJson			READ databaseJsonQ			WRITE setDatabaseJsonQ		NOTIFY databaseJsonChanged			)
	Q_PROPERTY(bool		dataFileSynch			READ dataFileSynchQ			WRITE setDataFileSynchQ		NOTIFY dataFileSynchChanged			)
	Q_PROPERTY(long		dataFileTimestamp		READ dataFileTimestamp		WRITE setDataTimestamp		NOTIFY dataTimestampChanged			)

	Q_PROPERTY(int		columnsFilteredCount	READ columnsFilteredCount								NOTIFY columnsFilteredCountChanged	)
	
friend ColumnQ;

public:
								DataSetQ(int index = -1);
								~DataSetQ();
	
	QHash<int, QByteArray>		roleNames()																						const	override;
			int					rowCount(		const QModelIndex &parent = QModelIndex())										const	override;
			int					columnCount(	const QModelIndex &parent = QModelIndex())										const	override;
			QVariant			data(			const QModelIndex &index, int role = Qt::DisplayRole)							const	override;
			bool				setData(		const QModelIndex &index, const QVariant &value, int role)								override;
			QVariant			headerData(		int section, Qt::Orientation orientation, int role = Qt::DisplayRole )			const	override;
			Qt::ItemFlags		flags(			const QModelIndex &index)														const	override;
			
			bool				insertRows(		int row,		int count, const QModelIndex & aparent = QModelIndex())					override;
			bool				insertColumns(	int column,		int count, const QModelIndex & aparent = QModelIndex())					override;
			bool				removeRows(		int row,		int count, const QModelIndex & aparent = QModelIndex())					override;
			bool				removeColumns(	int column,		int count, const QModelIndex & aparent = QModelIndex())					override;
			
	static	bool				dataMode();
	std::vector<ColumnQ*>	&	columnsQ() const;

	
			std::string			freeNewColumnName(size_t startHere)																const;
			bool				isColumnNameFree(const std::string & name)														const;
			
			QString				descriptionQ()		const;
			QString				dataFileQ()			const;
			QString				databaseJsonQ()		const;
			bool				dataFileSynchQ()	const;
			long				dataTimestamp()		const;
			
			void				setDescriptionQ(	const QString &	newDescription);
			void				setDataFileQ(		const QString &	newDataFile);
			void				setDatabaseJsonQ(	const QString &	newDatabaseJson);
			void				setDataFileSynchQ(	bool			newDataFileSynch);
			
			Column *			connectNewColumn(ColumnQ *newColumn);
			Filter *			connectNewFilter(FilterQ *newFilter);


			bool				dataFileCanHaveLabels() const;
			void				resetAllFilters();

			int					columnsFilteredCount() const;
			void				resetFilterCounters();
			void				resetVariableTypes();

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
			void				runFilter();
			void				allFiltersReset();
			void				showWarning(						QString title, QString msg);
			void				somethingModified();
			void				columnsBeingRemoved(				int columnIndex, int count);
			
			void				descriptionChanged();
			void				dataFileChanged();
			void				databaseJsonChanged();
			void				dataFileSynchChanged();
			void				dataTimestampChanged();
			void				columnsFilteredCountChanged();
			void				rowCountChanged();
			void				refreshAllAnalyses();

public slots:
			void				refresh()	{ beginResetModel(); endResetModel(); }
			void				handleColumnChanged(		ColumnQ * column);
			void				handleLabelsReordered(		ColumnQ * column);
			void				handleColumnTypeChanged(	ColumnQ * column);
			void				handleColumnsAboutToBeRemoved(const QModelIndex &parent, int first, int last) { emit columnsBeingRemoved(first, (last-first)+1); }

			bool				setColumnTypes(intset columnIndexes, columnType newColumnType);


private slots:
			void				handleDataSetChanged(	QStringList				changedColumns,
														QStringList				missingColumns,
														QMap<QString, QString>	changeNameColumns,
														bool					rowCountChanged,
														bool					hasNewColumns);
			
protected:
			QList<QVariant>		getColumnValuesAsDoubleList(int columnIndex)													const;
			bool				getRowFilter(int row)																			const;
			QVariant			getDataSetViewLines(bool up, bool left, bool down, bool right)									const;
			bool				getColumnInDragNDropShownFilter(int columnIndex)												const;
			bool				getColumnInDragNDropShownFilter(Column * column)												const;
			Column	*			_createColumn(int id = -1)																		override;
			Filter	*			_createFilter()																					override;
			Filter	*			_createFilter(const std::string & name, bool createIfMissing = true)							override;
			
};

#endif // DATASETQ_H
