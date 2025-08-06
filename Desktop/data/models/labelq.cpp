#include "labelq.h"
#include "columnq.h"

LabelQ::LabelQ(ColumnQ *column)
	: Label(column)
{
	
}

LabelQ::LabelQ(ColumnQ *column, int value)
	: Label(column, value)
{
	
}

LabelQ::LabelQ(ColumnQ *column, const std::string &label, int value, bool filterAllows, const std::string &description, const Json::Value &originalValue, int order, int id)
	: Label(column, label, value, filterAllows, description, originalValue, order, id)
{
	
}

void LabelQ::bindLambdaSignals()
{
	_emitLabelFilterChanged = [this](){ emit labelFilterChanged(); };
}
