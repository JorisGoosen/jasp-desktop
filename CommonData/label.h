#ifndef LABEL_H
#define LABEL_H

#include <string>
#include <json/json.h>
#include "datasetbasenode.h"
#include "columntype.h"

class Column;
class DatabaseInterface;

/// A label
/// 
/// Label is a class that stores the value of a column if it is not a Scale (a Nominal Int, Nominal Text, or Ordinal).
/// The original value can be an integer, float or string, this is stored in a json
///
/// Internally for all non-scalar columns they are stored as ints in Column::_ints, the value of a Label corresponds to that
/// Beyond that there are some extra attributes like a description or whether it is currently allowed by the generated filter.
///
/// The order of the labels in R is determined by their order in Column::_labels,
/// and Column makes sure (_dbUpdateLabelOrder) the order is stored in the database when it is changed.
class Label : public DataSetBaseNode
{
	Q_OBJECT
	
friend Column;

protected:
								Label(Column * column, const std::string & label, int value, bool filterAllows = true, const std::string & description = "", const Json::Value & originalValue = Json::nullValue, int order = -1, int id = -1); 

public:	
	static const int NO_LABEL;
	
			void				dbDelete();
			void				dbCreate();
			void				dbLoad(int labelId = -1);
			void				dbUpdate();
			
			int					rowCount(		const QModelIndex &parent = QModelIndex())										const	override;
			int					columnCount(	const QModelIndex &parent = QModelIndex())										const	override;
			QVariant			data(			const QModelIndex &index, int role = Qt::DisplayRole)							const	override;

			Label			&	operator=(const Label &label);
			
			int					dbId()						const	{ return _dbId;				}
			bool				userAdded()					const	{ return _userAdded;		}
	const	std::string		&	description()				const	{ return _description;		}
			std::string			label()						const	{ return _label;			}
			std::string			labelDisplay()				const;
			int					intsId()					const	{ return _intsId;			}
			bool				isEmptyValue()				const;
			int					order()						const	{ return _order;			}
			bool				filterAllows()				const	{ return _filterAllows;		}
	const	Json::Value		&	originalValue()				const	{ return _originalValue;	}
			double				originalValueAsDouble()		const	{ return _dblValue;			}
	std::pair<std::string
		,std::string>			origValDisplay()			const	{ return std::make_pair(originalValueAsString(), label()); }
			std::string			getValue(	bool fancyEmptyValue = false, bool ignoreEmptyValue = false, bool sepas = true, columnType asType = columnType::unknown)	const; ///< Returns the ("original") value. Basically whatever the user would like to see as value. Stored internally as json
			std::string			getDisplay(	bool fancyEmptyValue = true, bool sepas = true)									const;
			std::string			getShadow(	bool fancyEmptyValue = true, bool sepas = true)									const;
			std::string			getLabel(	bool ignoreEmptyValue = false)	const;

	static	std::string			originalValueAsString(const Column * column, const Json::Value & originalValue, bool fancyEmptyValue = false, bool ignoreEmpty=true);
			std::string			originalValueAsString(bool fancyEmptyValue = false, bool ignoreEmpty = true)		const;
			std::string			str() const;
			
			void				setIntsId(			int value);
			void				setOrder(			int order);
			void				setDbId(			int id) { _dbId = id; }
			bool				setLabel(			const std::string & label);
			bool				setOriginalValue(	const Json::Value & originalValue);
			bool				setOrigValLabel(	const Json::Value & originalValue);
			bool				setDescription(		const std::string & description);
			bool				setFilterAllows(	bool allowFilter);
			void				setUserAdded(		bool userAddedIt);
			void				setInformation(		Column * column, int id, int order, const std::string &label, int value, bool filterAllows, const std::string & description, const Json::Value & originalValue);
			
			void				updateDoubleLabelsPostLocaleChange();

			Json::Value			serialize()	const;

			DatabaseInterface	& db();
	const	DatabaseInterface	& db() const;
	
signals:
	void				manualEditMade();
	void				labelFilterChanged();


private:
			void				_setOriginalValue(	const Json::Value & originalValue);

	Column		*	_column;

	Json::Value		_originalValue	= Json::nullValue;	///< Could contain integers, floats or strings. Arrays and objects are undefined.
	
	int				_dbId			= -1,	///< Database id
					_order			= -1,	///< Should correspond to its position in Column::_labels
					_intsId			= -1;	///< value of label, should always map to Column::_ints
	std::string		_label,					///< What to display in the dataview
					_description;			///< Extended information for tooltip in dataview and of course in the variableswindow
	bool			_filterAllows	= true,	///< Used in generating filters for when users disable and enable certain labels/levels
					_userAdded		= false;
	double			_dblValue;
};

typedef std::vector<Label*>				Labels;
typedef std::set<Label*>				Labelset;

#endif // LABEL_H
