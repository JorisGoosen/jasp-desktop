#ifndef FILTERMODEL_H
#define FILTERMODEL_H

#include <set>
#include <QObject>
#include <QString>

class FilterQ;
class UndoStack;

///
/// Passthrough for the filter gui
class FilterModel : public QObject
{
	Q_OBJECT


public:
	explicit					FilterModel(QObject * parent = nullptr);

				FilterQ		*	filter()				const;
				void			init();

				QString			rFilter()				const;
				QString			constructorR()			const;
				QString			statusBarText()			const;
				QString			filterErrorMsg()		const;
				QString			generatedFilter()		const;
				QString			constructorJson()		const;
	static		const char *	defaultRFilter();
				bool			hasFilter()				const;

				void			runFilter();

public slots:
	void applyConstructorJson(	QString constructorJson);
	void applyRFilter(			QString rFilter);

	void processFilterDone();

	void computeColumnSucceeded(QString columnName, QString warning, bool dataChanged);

signals:
	void rFilterChanged();
	void hasFilterChanged();
	void constructorRChanged();
	void statusBarTextChanged();
	void filterErrorMsgChanged();
	void generatedFilterChanged();
	void constructorJsonChanged();

};

#endif // FILTERMODEL_H
