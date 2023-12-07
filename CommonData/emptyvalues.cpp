#include "emptyvalues.h"
#include "columnutils.h"
#include "log.h"
#include "jsonutilities.h" 

std::string EmptyValues::_displayString = "";

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


const stringset& EmptyValues::emptyValues() const
{
	static stringset localSet;
			
	if(!_parent)
		return _emptyStrings;
	
	localSet = _parent->emptyValues();
	localSet.insert(_emptyStrings.begin(), _emptyStrings.end());
	
	return localSet;
}

const doubleset & EmptyValues::doubleEmptyValues() const
{
	static doubleset localSet;
			
	if(!_parent)
		return _emptyDoubles;
	
	localSet = _parent->doubleEmptyValues();
	localSet.insert(_emptyDoubles.begin(), _emptyDoubles.end());
	
	return localSet;
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



