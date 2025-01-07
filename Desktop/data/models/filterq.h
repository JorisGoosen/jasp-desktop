#ifndef FILTERQ_H
#define FILTERQ_H

#include "filter.h"
#include <QObject>

class DataSetQ;
class LabelFilterGenerator;

class FilterQ : public Filter, public QObject
{
	Q_OBJECT
	friend DataSetQ;

	Q_PROPERTY( QString name				READ nameQ											NOTIFY nameChanged				)
	Q_PROPERTY( QString generatedFilter		READ generatedFilter	WRITE setGeneratedFilter	NOTIFY generatedFilterChanged	)
	Q_PROPERTY( QString rFilter				READ rFilter			WRITE setRFilter			NOTIFY rFilterChanged			)
	Q_PROPERTY( QString constructorJson		READ constructorJson	WRITE setConstructorJson	NOTIFY constructorJsonChanged	)
	Q_PROPERTY( QString constructorR		READ constructorR		WRITE setConstructorR		NOTIFY constructorRChanged		)
	Q_PROPERTY( QString statusBarText		READ statusBarText									NOTIFY statusBarTextChanged		)
	Q_PROPERTY( QString filterErrorMsg		READ filterErrorMsg									NOTIFY filterErrorMsgChanged	)
	Q_PROPERTY( bool 	hasFilter			READ hasFilter										NOTIFY hasFilterChanged			)
	Q_PROPERTY( QString defaultRFilter		READ defaultRFilter									NOTIFY defaultRFilterChanged	)
	Q_PROPERTY( int		filteredRowCount	READ filteredRowCount								NOTIFY filteredRowCountChanged	)
	Q_PROPERTY( bool	invalidated			READ invalidated									NOTIFY invalidatedChanged		)


protected:
							FilterQ(DataSetQ * data); ///< Dont use directly! Use DataSet::_createFilter
							FilterQ(DataSetQ * data, const std::string & name, bool createIfMissing = true); ///< Dont use directly! Use DataSet::_createFilter

public:
			DataSetQ	*	dataQ()					const;
			QString			nameQ()					const;
			QString			rFilterQ()				const;
			QString			constructorRQ()			const;
			QString			statusBarText()			const	{ return _statusBarText;			}
			QString			filterErrorMsgQ()		const;
			QString			generatedFilterQ()		const;
			QString			constructorJsonQ()		const;

			bool			columnUsed(const QString & name) const;

	static	const char *	defaultRFilter();

			bool			hasFilter()				const;

			void			setRFilter(			const QString & newRFilter			);
			void			setConstructorR(	const QString & newConstructorR		);
			void			setGeneratedFilter(	const QString & newGeneratedFilter	);
			void			setConstructorJson(	const QString & newconstructorJson	);
			void			setFilterErrorMsg(	const QString & newFilterErrorMsg	);
			void			setStatusBarText(	const QString & newStatusBarText	);

	
signals:
			void			nameChanged();
			void			rFilterChanged();
			void			filteredChanged();
			void			updateStatusBar();
			void			hasFilterChanged();
			void			invalidatedChanged();
			void			dataSetShouldRefresh();
			void			constructorRChanged();
			void			statusBarTextChanged();
			void			filterErrorMsgChanged();
			void			generatedFilterChanged();
			void			constructorJsonChanged();
			void			filteredRowCountChanged();

protected slots:
			void datasetChanged(QStringList changedColumns, QStringList missingColumns, QMap<QString, QString> changeNameColumns, bool rowCountChanged, bool);

private:
			void						bindStdFunctionsToSignals();
			QString						_statusBarText;

			LabelFilterGenerator	*	_labelGen	= nullptr;

};

#endif // FILTERQ_H
