#ifndef EMPTYVALUES_H
#define EMPTYVALUES_H

#include "json/json.h"
#include "utils.h"
#include "stringutils.h"

class EmptyValues
{
public:
								EmptyValues(EmptyValues * parent = nullptr);
								~EmptyValues();
								
			void				resetEmptyValues();

			void				fromJson(				const Json::Value	& json);
			Json::Value			toJson() const;
			
            bool				isEmptyValue(const std::string & data)				const;
            bool				isEmptyValue(double				data)           	const;
			
	const	stringset		&	emptyStrings()										const;
	const	doubleset		&	doubleEmptyValues()									const;
			bool				hasCustomEmptyValues()								const;
            void				setEmptyValues(			const stringset	& values);
			void				setHasCustomEmptyValues(bool hasCustom);
			
	static	void				setDisplayString(const std::string & str);
	static	std::string		&	displayString() { return _displayString; }

	static	const int			missingValueInteger;
	static	const double		missingValueDouble;

private:
	
	
private:
	static	std::string			_displayString;
			EmptyValues		*	_parent					= nullptr;
			stringset			_emptyStrings;
			doubleset			_emptyDoubles;
			bool				_hasCustomEmptyValues	= false; ///< Only used by EmptyValues with a parent
};

#endif // EMPTYVALUES_H
