#ifndef COLUMNQ_H
#define COLUMNQ_H

#include <column.h>
#include <QAbstractTableModel>

class LabelQ;
class DataSetQ;
class ColumnQ : public Column, public QAbstractTableModel
{
	Q_OBJECT
friend DataSetQ;
protected:
	ColumnQ(DataSetQ * data, int id = -1); ///< Dont use directly! Use DataSet::_createFilter

public:
	~ColumnQ();
	
	QHash<int, QByteArray>		roleNames()																						const	override;
			int					rowCount(		const QModelIndex &parent = QModelIndex())										const	override;
			int					columnCount(	const QModelIndex &parent = QModelIndex())										const	override;
			QVariant			headerData(		int section, Qt::Orientation orientation, int role = Qt::DisplayRole )			const	override;
			QVariant			data(			const QModelIndex &index, int role = Qt::DisplayRole)							const	override;
			bool				setData(		const QModelIndex &index, const QVariant &value, int role)								override;
			
			void				refresh()	{ beginResetModel(); endResetModel(); }
			DataSetQ		*	dataQ()							const;
			QList<QVariant>		getColumnValuesAsDoubleList()	const;

			bool				setLabelDescription(int labelRow, const QString &	newDescription	);
			bool				setLabelDisplay(	int labelRow, const QString &	newLabel		);
			bool				setLabelValue(		int labelRow, const QString &	newLabelValue	);
			bool				setLabelAllowFilter(int labelRow, bool				newAllowValue	);

			std::string			generateLabelFilter() const;

protected:
			boolvec				getFilterAllows() const;
			void				resetFilterAllows();
			Label *				connectNewLabel(LabelQ *newLabel);
	virtual	Label *				_createLabel(
							const std::string & label, 
							int					value, 
							bool				filterAllows	= true, 
							const std::string & description		= "", 
							const Json::Value & originalValue	= Json::nullValue, 
							int					order			= -1, 
							int					id				= -1)														override;
			

			

signals:
			void				manualEditMade();
			void				dataSetShouldRefresh();
			void				columnChanged(		ColumnQ * column);
			void				columnTypeChanged(	ColumnQ * column);
			void				labelsReordered(	ColumnQ * column);
			void				labelFilterChanged();
			void				runFilter();
			void				showWarning(						QString title, QString msg);
};

typedef std::vector<ColumnQ*> ColumnsQ;
#endif // COLUMNQ_H
