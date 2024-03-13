#include "emptyvalues.h"
#include "columnutils.h"
#include "log.h"
#include "jsonutilities.h"

std::string		EmptyValues::_displayString			= "";
const int		EmptyValues::missingValueInteger	= std::numeric_limits<int>::lowest();
const double	EmptyValues::missingValueDouble		= NAN;

EmptyValues::EmptyValues(EmptyValues * parent) : _parent(parent)
{
	//Log::log() << "EmptyValues::EmptyValues(" << (parent ? "a parent" : "null") << ");" << std::endl;
}

void EmptyValues::resetEmptyValues()
{
	_emptyStrings.clear();
	_emptyDoubles.clear();
}

EmptyValues::~EmptyValues()
{

}

Json::Value EmptyValues::toJson() const
{
	return JsonUtilities::setToJsonArray(_emptyStrings);
}

void EmptyValues::fromJson(const Json::Value & json)
{
	resetEmptyValues();
	
	if (!json.isArray())
		return;
	
	setEmptyValues(JsonUtilities::jsonStringArrayToSet(json));
}


const stringset& EmptyValues::emptyStrings() const
{
	return _emptyStrings;
}

const doubleset & EmptyValues::emptyDoubles() const
{
    return _emptyDoubles;
}

void EmptyValues::setEmptyValues(const stringset& values)
{
	_emptyStrings	= values;
	_emptyDoubles	= ColumnUtils::getDoubleValues(values);
	_hasEmptyValues	= _hasEmptyValues || _emptyStrings.size(); //Dont set it to false if it is true, and set it to true if there are empty strings set at all
}

bool EmptyValues::hasEmptyValues() const
{
	return !_parent || _hasEmptyValues;
}

void EmptyValues::setHasCustomEmptyValues(bool hasThem)
{
	if(_hasEmptyValues == hasThem)
		return;
	
	_hasEmptyValues = hasThem;
	
	if(_parent)
		setEmptyValues(!_hasEmptyValues ? stringset() : _parent->_emptyStrings);
		
}

bool EmptyValues::isEmptyValue(const std::string& val) const
{
	return hasEmptyValues() ? _emptyStrings.count(val) : ( _parent && _parent->isEmptyValue(val));
}

bool EmptyValues::isEmptyValue(const double val) const
{
	return hasEmptyValues() ? std::isnan(val) || _emptyDoubles.count(val) : ( _parent && _parent->isEmptyValue(val));
}
