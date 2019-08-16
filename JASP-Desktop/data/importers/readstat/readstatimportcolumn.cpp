#include "readstatimportcolumn.h"
#include "utils.h"
#include "log.h"

using namespace std;

ReadStatImportColumn::ReadStatImportColumn(ImportDataSet* importDataSet, string name, std::string labelsID, columnType columnType)
	: ImportColumn(importDataSet, name), _labelsID(labelsID), _type(columnType)
{}

ReadStatImportColumn::~ReadStatImportColumn()
{}

size_t ReadStatImportColumn::size() const
{
	switch(_type)
	{
	default:							return 0;
	case columnType::ColumnTypeScale:		return _doubles.size();
	case columnType::ColumnTypeOrdinal:		[[clang::fallthrough]];
	case columnType::ColumnTypeNominal:		return _ints.size();
	case columnType::ColumnTypeNominalText:	return _strings.size();
	}
}

std::string ReadStatImportColumn::valueAsString(size_t row) const
{
	if(row >= size())	return Utils::emptyValue;

	switch(_type)
	{
	default:							return Utils::emptyValue;
	case columnType::ColumnTypeScale:		return std::to_string(_doubles[row]);
	case columnType::ColumnTypeOrdinal:		[[clang::fallthrough]];
	case columnType::ColumnTypeNominal:		return std::to_string(_ints[row]);
	case columnType::ColumnTypeNominalText:	return _strings[row];
	}

}


std::vector<std::string> ReadStatImportColumn::allValuesAsStrings() const
{
	std::vector<std::string> strs;
	strs.reserve(size());

	for(size_t row = 0; row<size(); row++)
		strs.push_back(valueAsString(row));

	return strs;
}


void ReadStatImportColumn::addValue(const string & val)
{
	if(_ints.size() == 0 && _doubles.size() == 0)
		_type = columnType::ColumnTypeNominalText; //If we haven't added anything else and the first value is a string then the rest should also be a string from now on

	switch(_type)
	{
	case columnType::ColumnTypeUnknown:
		_type = columnType::ColumnTypeNominalText;
		[[clang::fallthrough]];

	case columnType::ColumnTypeNominalText:
		_strings.push_back(val);
		break;

	case columnType::ColumnTypeScale:
	{
		double dblVal;
		if(Utils::convertValueToDoubleForImport(val, dblVal))	addValue(dblVal);
		else													addMissingValue();

		break;
	}

	case columnType::ColumnTypeOrdinal:
	case columnType::ColumnTypeNominal:
	{
		int intVal;
		if(Utils::convertValueToIntForImport(val, intVal))	addValue(intVal);
		else												addMissingValue();

		break;
	}
	}
}

void ReadStatImportColumn::addValue(const double & val)
{
	switch(_type)
	{
	case columnType::ColumnTypeUnknown:
		_type = columnType::ColumnTypeScale;
		[[clang::fallthrough]];

	case columnType::ColumnTypeScale:
		_doubles.push_back(val);
		break;

	case columnType::ColumnTypeNominalText:
		addValue(std::to_string(val));
		break;

	case columnType::ColumnTypeOrdinal:
	case columnType::ColumnTypeNominal:
		addValue(int(val));
		break;
	}
}

void ReadStatImportColumn::addValue(const int & val)
{
	switch(_type)
	{
	case columnType::ColumnTypeUnknown:
		_type = columnType::ColumnTypeOrdinal;
		[[clang::fallthrough]];

	case columnType::ColumnTypeOrdinal:
	case columnType::ColumnTypeNominal:
		_ints.push_back(val);
		break;

	case columnType::ColumnTypeNominalText:
		addValue(std::to_string(val));
		break;

	case columnType::ColumnTypeScale:
		addValue(double(val));
		break;
	}
}

void ReadStatImportColumn::addLabel(const int & val, const std::string & label)
{
	if(!(_type == columnType::ColumnTypeOrdinal || _type == columnType::ColumnTypeNominal))
		Log::log() << "Column type being imported through readstat is not ordinal or nominal but receives an int as value for label " << label << std::endl;
	else
		_intLabels[val] = label;
}

void ReadStatImportColumn::addLabel(const std::string & val, const std::string & label)
{
	if(_ints.size() == 0 && _doubles.size() == 0)
		_type = columnType::ColumnTypeNominalText; //If we haven't added anything else and the first value is a string then the rest should also be a string from now on

	if(_type != columnType::ColumnTypeNominalText)
		Log::log() << "Column type being imported through readstat is not nominal-text but receives a string as value for label " << label << std::endl;
	else
		_strLabels[val] = label;
}

void ReadStatImportColumn::addMissingValue()
{
	switch(_type)
	{
	case columnType::ColumnTypeUnknown:												return;
	case columnType::ColumnTypeScale:		_doubles.push_back(NAN);				return;
	case columnType::ColumnTypeOrdinal:		[[clang::fallthrough]];
	case columnType::ColumnTypeNominal:		_ints.push_back(INT_MIN);				return;
	case columnType::ColumnTypeNominalText:	_strings.push_back(Utils::emptyValue);	return;
	}
}

void ReadStatImportColumn::addValue(const readstat_value_t & value)
{
	readstat_type_t			type = readstat_value_type(value);

	if (!readstat_value_is_system_missing(value))
		switch(type)
		{
		case READSTAT_TYPE_STRING:		addValue(			readstat_string_value(value)	);	return;
		case READSTAT_TYPE_INT8:		addValue(int(		readstat_int8_value(value))		);	return;
		case READSTAT_TYPE_INT16:		addValue(int(		readstat_int16_value(value))	);	return;
		case READSTAT_TYPE_INT32:		addValue(int(		readstat_int32_value(value))	);	return;
		case READSTAT_TYPE_FLOAT:		addValue(double(	readstat_float_value(value))	);	return;
		case READSTAT_TYPE_DOUBLE:		addValue(			readstat_double_value(value)	);	return;
		}

	addMissingValue();
}
