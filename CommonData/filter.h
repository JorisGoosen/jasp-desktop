#ifndef FILTER_H
#define FILTER_H

#include "datasetbasenode.h"
#include <string>
#include <vector>
#include "utils.h"

#define DEFAULT_FILTER_JSON	"{\"formulas\":[]}"
#define DEFAULT_FILTER_GEN	"generatedFilter <- rep(TRUE, rowcount)"


class DataSet;
class DatabaseInterface;

///Interface to sqlite Filters table
///
/// It both stores the values of the filter, it also stores the R-filter constructor filter and errormsgs.
/// Instead of sending all the data through json we now just tell the desktop when we are finished.
/// "revision" and sqlite then make sure it gets properly synchronized in Desktop
///
/// If a filter has a name it is used by an analysis only, if not it is part of the DataSet and coupled with the GUI
/// This means the user can (when the filter is selected) dis/enable labels and they become part of this filter.
/// The same goes for drag'n'drop filter and or Rfilter
/// Perhaps later this will be done in a different way
/// (maybe the user-gui editable filters also need a name or something later, although I guess a title/description is probably better in that case)
class Filter : public DataSetBaseNode
{
	friend DataSet;
protected:
	Filter(DataSet * data); ///< Dont use directly! Use DataSet::_createFilter
	Filter(DataSet * data, const std::string & name, bool createIfMissing = true); ///< Dont use directly! Use DataSet::_createFilter

public:
	DataSet					*	data()				const { return _data;					}
	int							id()				const { return _id;						}
	const std::string		&	name()				const { return _name;					}
	bool						isDataSetFilter()	const { return _name.empty();			} ///< If the Filter has a name it is created by an analysis or something. Otherwise it represents a (possible) combination of a drag'n'drop filter, labels-filter and/or R-filter as manually entered in the GUI
	const std::string		&	rFilter()			const { return _rFilter;				}
	const std::string		&	generatedFilter()	const { return _generatedFilter;		}
	const std::string		&	constructorJson()	const { return _constructorJson;		}
	const std::string		&	constructorR()		const { return _constructorR;			}
	bool						invalidated()		const { return _invalidated;			}
	const std::string		&	errorMsg()			const { return _errorMsg;				}
	const std::vector<bool>	&	filtered()			const { return _filtered;				}
	int							filteredRowCount()	const { return _filteredRowCount;		}

	void						setRFilter(			const std::string	& rFilter);
	void						setGeneratedFilter(	const std::string	& generatedFilter);
	void						setConstructorJson(	const std::string	& constructorJson);
	void						setConstructorR(	const std::string	& constructorR);
	void						setInvalidated(		bool					invalidated);
	void						setErrorMsg(		const std::string	& errorMsg);
	void						setName(			const std::string	& name);
	bool						setFilterVector(	const boolvec		& filterResult);
	void						setFilterValueNoDB(	size_t	row, bool val);
	void						setRowCount(		size_t	rows);
	void						setId(				int		id)			{ _id = id; }

	void						dbCreate();
	void						dbUpdate();
	void						dbUpdateErrorMsg();
	void						dbLoad();
	
	void						dbDelete();
	void						incRevision() override;
	bool						checkForUpdates();


	stringset					columnsUsedInConstructor()	const;
	stringset					columnsUsedInRFilter()		const;

	static bool					filterNameIsFree(const std::string & filterName);

	void						reset();

	DatabaseInterface		&	db();
	const DatabaseInterface	&	db() const;
	
protected:
	void						dbLoadResultAndError();					///< Loads (updated) filtervalues from database and the (possible) error msg, returns true if an error is set
	void						calculateFilteredRowCount();
	void						rescanForColumns();

	std::function<void()>		_nameChanged;
	std::function<void()>		_rFilterChanged;
	std::function<void()>		_filteredChanged;
	std::function<void()>		_generatedFilterChanged;
	std::function<void()>		_filteredRowCountChanged;
	std::function<void()>		_constructorJsonChanged;
	std::function<void()>		_constructorRChanged;
	std::function<void()>		_invalidatedChanged;
	std::function<void()>		_errorMsgChanged;


private:
	DataSet					*	_data				= nullptr;
	int							_id					= -1,
								_filteredRowCount	= 0;
	std::string					_rFilter			= "",
								_generatedFilter	= "",
								_constructorJson	= "",
								_constructorR		= "",
								_errorMsg			= "",
								_name				= "";
	bool						_invalidated		= false;
	boolvec						_filtered;
	stringset					_columnsInConstructorJson,
								_columnsUsedInRFilter;
};

#endif // FILTER_H
