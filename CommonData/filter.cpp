#include "filter.h"
#include "timers.h"
#include "dataset.h"
#include "jsonutilities.h"
#include "databaseinterface.h"

Filter::Filter(DataSet * data)
	: DataSetBaseNode(dataSetBaseNodeType::filter, data), _data(data)
{ }

Filter::Filter(DataSet * data, const std::string & name, bool createIfMissing)
	: DataSetBaseNode(dataSetBaseNodeType::filter), _data(data), _name(name)
{
	assert(_name != "");

	if(db().filterGetId(_name) > -1)	dbLoad();
	else if(createIfMissing)			dbCreate();
	else								throw std::runtime_error("Filter by name '" + _name + "' but it doesnt exist and createIfMissing=false!\nAre you sure this filter should exist?");
}

void Filter::dbCreate()
{
	assert(_id == -1);
	_id = db().filterInsert(_data->id(), _rFilter, _generatedFilter, _constructorJson, _constructorR, _name);
}

void Filter::dbUpdate()
{
	JASPTIMER_SCOPE(Filter::dbUpdate);

	assert(_id != -1);

	db().transactionWriteBegin();
	if(!_data->writeBatchedToDB())
		db().filterUpdate(_id, _rFilter, _generatedFilter, _constructorJson, _constructorR, _name, _invalidated);

	incRevision();
	db().transactionWriteEnd();
}

void Filter::dbUpdateErrorMsg()
{
	assert(_id != -1);
	db().transactionWriteBegin();
	if(!_data->writeBatchedToDB())
	{
		auto oldError = _errorMsg;
		db().filterUpdateErrorMsg(_id, _errorMsg);
		if(oldError != _errorMsg)
			_errorMsgChanged();
	}
	incRevision();
	db().transactionWriteEnd();
}

void Filter::dbLoad()
{
	if(_id == -1)
		_id = _name == "" ? db().filterGetId(_data->id()) : db().filterGetId(_name);

	if(_id == -1)
		return;

	db().transactionReadBegin();
	
	auto	oldRFilter			= _rFilter,
			oldGeneratedFilter	= _generatedFilter,
			oldConstructorJson	= _constructorJson,
			oldConstructorR		= _constructorR;

	std::string nameInDB = "";
	db().filterLoad(_id, _rFilter, _generatedFilter, _constructorJson, _constructorR, _revision, nameInDB, _invalidated);
	assert(nameInDB == _name);

	rescanForColumns();

	if(oldRFilter			!= _rFilter)			_rFilterChanged();
	if(oldGeneratedFilter	!= _generatedFilter)	_generatedFilterChanged();
	if(oldConstructorJson	!= _constructorJson)	_constructorJsonChanged();
	if(oldConstructorR		!= _constructorR)		_constructorRChanged();
	
	dbLoadResultAndError();

	db().transactionReadEnd();
}

bool Filter::setFilterVector(const boolvec & filterResult)
{
	bool changed = false;

	if(_filtered.size() == 0)
	{
		_filtered = filterResult;
		changed = true;
	}
	else
		for(size_t i=0; i<filterResult.size(); i++)
		{
			if(_filtered[i] != filterResult[i])
				changed = true;

			_filtered[i] = filterResult[i];
		}

	if(!_data->writeBatchedToDB())
		db().filterWrite(_id, _filtered);

	calculateFilteredRowCount();

	if(changed)
		incRevision();

	return changed;
}

void Filter::setFilterValueNoDB(size_t row, bool val)
{
	_filtered[row] = val;
}

void Filter::setRowCount(size_t rows)
{
	_filtered.resize(rows);
}

void Filter::dbLoadResultAndError()
{
	assert(_id != -1);
	
	_errorMsg = db().filterLoadErrorMsg(_id);

	 if(db().filterSelect(_id, _filtered))
		_filteredChanged(); 
	 
	 calculateFilteredRowCount();
	 

}

void Filter::dbDelete()
{
	assert(_id != -1);

	db().filterDelete(_id);
	_id = -1;
}

void Filter::incRevision()
{
	assert(_id != -1);
	
	if(!_data->writeBatchedToDB())
	{
		_revision = db().filterIncRevision(_id);
		checkForChanges();
	}
}

bool Filter::checkForUpdates()
{
	if(_id == -1)
	{
		_id = db().dataSetGetFilter(_data->id());
		
		if(_id == -1)
			return false;
	}
	else if(_revision == db().filterGetRevision(_id))
		return false;

	if(_data->id() != -1 && _id != -1)
	{
		dbLoad();
		return true;
	}
	else
		return false;
}

void Filter::setName(const std::string &name)
{
	bool	wasChange	=_name != name;
			_name		= name;

	dbUpdate();

	if(wasChange)
		_nameChanged();
}

void Filter::setRFilter(const std::string &rFilter)
{
	bool	wasChange	=_rFilter != rFilter;
			_rFilter	= rFilter;

	rescanForColumns();

	dbUpdate();

	if(wasChange)
		_rFilterChanged();
}

void Filter::calculateFilteredRowCount()
{
	int newRowCount = 0;
	for(bool f : _filtered)
		if(f)
			newRowCount++;

	bool wasChange = newRowCount != _filteredRowCount;
	_filteredRowCount = newRowCount;

	if(wasChange)
		_filteredRowCountChanged();
}

void Filter::setGeneratedFilter(const std::string &generatedFilter)
{
	bool	wasChange			=_generatedFilter != generatedFilter;
			_generatedFilter	= generatedFilter;

	dbUpdate();

	if(wasChange)
		_generatedFilterChanged();
}


void Filter::setConstructorJson(const std::string &constructorJson)	
{ 
	bool	wasChange					=_constructorJson != constructorJson;
			_constructorJson			= constructorJson;

	rescanForColumns();

	dbUpdate(); 

	if(wasChange)
		_constructorJsonChanged();
}

void Filter::setConstructorR(const std::string &constructorR)
{
	bool	wasChange		=_constructorR != constructorR;
			_constructorR	= constructorR;

	dbUpdate();

	if(wasChange)
		_constructorRChanged();
}

void Filter::setInvalidated(bool invalidated)
{
	bool	wasChange		=_invalidated != invalidated;
			_invalidated	= invalidated;

	dbUpdate();

	if(wasChange)
		_invalidatedChanged();

}

void Filter::setErrorMsg(const std::string &errorMsg)
{
	bool	wasChange	= _errorMsg != errorMsg;
			_errorMsg	= errorMsg;

	dbUpdateErrorMsg();

	if(wasChange)
		_errorMsgChanged();
}

stringset Filter::columnsUsedInConstructor() const
{
	return _columnsInConstructorJson;
}

stringset Filter::columnsUsedInRFilter() const
{
	return _columnsUsedInRFilter;
}

bool Filter::filterNameIsFree(const std::string &filterName)
{
	return -1 == DatabaseInterface::singleton()->filterGetId(filterName);
}

void Filter::reset()
{
	if(!_data->writeBatchedToDB())
		db().filterClear(_id);

	incRevision();
	_filtered = boolvec(_data->rowCount(), true);
}

DatabaseInterface		& Filter::db()			{ return *DatabaseInterface::singleton(); }
const DatabaseInterface & Filter::db() const	{ return *DatabaseInterface::singleton(); }

void Filter::rescanForColumns()
{
	_columnsUsedInRFilter		= data()->findUsedColumnNames(_rFilter);
	_columnsInConstructorJson	= JsonUtilities::convertDragNDropFilterJSONToSet(_constructorJson);
}
