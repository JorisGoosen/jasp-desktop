#ifndef FILTERMODEL_H
#define FILTERMODEL_H

#include <set>
#include <QObject>
#include <QString>

class Filter;
class UndoStack;

///
/// Passthrough for the filter gui
class FilterModel : public QObject
{
	Q_OBJECT

	Q_PROPERTY( QVariantList filterDropDownList READ filterDropDownList NOTIFY filterDropDownListChanged)
	Q_PROPERTY( Filter	*	filter				READ filter				NOTIFY filterChanged)

public:
	explicit					FilterModel(QObject * parent = nullptr);

				Filter		*	filter()				const;

				QVariantList	filterDropDownList()	const;
				bool			hasFilter()				const;

	Q_INVOKABLE bool			isJustGeneratedFilter() const;
				
signals:
				void			filterDropDownListChanged();
				void			filterChanged();

public slots:
	void applyConstructorJson(	QString constructorJson);
	void applyRFilter(			QString rFilter);
	void computeColumnSucceeded(QString columnName, QString warning, bool dataChanged);
	void processFilterResult();
	void processFilterErrorMsg();
};

#endif // FILTERMODEL_H
