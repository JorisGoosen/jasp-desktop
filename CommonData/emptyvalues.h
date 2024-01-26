#ifndef EMPTYVALUES_H
#define EMPTYVALUES_H

#include "utils.h"
#include "json/value.h"

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
			const	doubleset		&	emptyDoubles()									const;
			bool				hasEmptyValues()									const;
			void				setHasCustomEmptyValues(bool hasThem);
		    void				setEmptyValues(const stringset	& values);
			
			static	void		setDisplayString(const std::string & str)	{ _displayString = str;}
	static	std::string		&	displayString()								{ return _displayString; }

	static	const int			missingValueInteger;
	static	const double		missingValueDouble;

private:
	
	
private:
	static	std::string			_displayString;
			EmptyValues		*	_parent					= nullptr;
			stringset			_emptyStrings;
			doubleset			_emptyDoubles;
			bool				_hasEmptyValues			= false;
};

#endif // EMPTYVALUES_H
