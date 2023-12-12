#include "emptyvalues.h"
#include "columnutils.h"
#include "log.h"
#include "jsonutilities.h"

std::string		EmptyValues::_displayString			= "";
const int		EmptyValues::missingValueInteger	= std::numeric_limits<int>::lowest();
const double	EmptyValues::missingValueDouble		= NAN;

EmptyValues::EmptyValues(EmptyValues * parent) : _parent(parent)
{

}

void EmptyValues::resetEmptyValues()
{
	_emptyStrings.clear();
	_emptyDoubles.clear();
	_hasCustomEmptyValues = false;
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

const doubleset & EmptyValues::doubleEmptyValues() const
{
    return _emptyDoubles;
}

void EmptyValues::setEmptyValues(const stringset& values)
{
	_emptyStrings = values;
	_emptyDoubles = ColumnUtils::getDoubleValues(values);
	
	if(_parent)
		_hasCustomEmptyValues = true;
}


bool EmptyValues::hasCustomEmptyValues() const
{
	return !_parent || _hasCustomEmptyValues;
}


void EmptyValues::setHasCustomEmptyValues(bool hasCustom)
{
	_hasCustomEmptyValues = hasCustom;
}


bool EmptyValues::isEmptyValue(const std::string& val) const
{
	return _emptyStrings.count(val) || ( _parent && _parent->isEmptyValue(val));
}

bool EmptyValues::isEmptyValue(const double val) const
{
	return _emptyDoubles.count(val) || ( _parent && _parent->isEmptyValue(val));
}
