#ifndef LABELQ_H
#define LABELQ_H

#include <label.h>
#include <QObject>

class ColumnQ;
class LabelQ : public Label
{
	Q_OBJECT
public:
	LabelQ(ColumnQ * column);
	LabelQ(ColumnQ * column, int value);
	LabelQ(ColumnQ * column, const std::string & label, int value, bool filterAllows = true, const std::string & description = "", const Json::Value & originalValue = Json::nullValue, int order = -1, int id = -1);

	void bindLambdaSignals();
	
signals:
	void				manualEditMade();
	void				labelFilterChanged();
};

#endif // LABELQ_H
