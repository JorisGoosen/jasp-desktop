#include "optionencodablestring.h"



Option * OptionEncodableString::clone() const
{
	OptionEncodableString *c = new OptionEncodableString();
	c->setValue(_value);
	return c;
}

Json::Value OptionEncodableString::asMetaJSON() const
{
	Json::Value meta	= defaultMetaEntryContainingColumn();
	meta["encodeThis"]	= _value;

	return meta;
}
