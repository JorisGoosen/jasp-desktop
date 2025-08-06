#include <cassert>
#include "filter.h"
#include "timers.h"
#include "qutils.h"
#include "dataset.h"
#include "dataenums.h"
#include "columnencoder.h"
#include "jsonutilities.h"
#include "databaseinterface.h"
#include "labelfiltergenerator.h"

Filter::Filter(DataSet * data)
: DataSetBaseNode(dataSetBaseNodeType::filter, data), 
  _data(				data), 
  _name(				DEFAULT_FILTER_NAME),
  _constructorJson(		DEFAULT_FILTER_JSON),
  _generatedFilter(		DEFAULT_FILTER_GEN)
{ 
	connectionCreation();
	
	_rFilter			= fq(defaultRFilter());
	_labelGen			= new LabelFilterGenerator(this);
}

Filter::Filter(DataSet * data, const std::string & name, bool createIfMissing)
	: DataSetBaseNode(dataSetBaseNodeType::filter), _data(data), _name(name)
{
	assert(_name != "");
	
	if(db().filterGetId(_name) > -1)	dbLoad();
	else if(createIfMissing)			dbCreate();
	else								throw std::runtime_error("Filter by name '" + _name + "' but it doesnt exist and createIfMissing=false!\nAre you sure this filter should exist?");
	
	connectionCreation();
}


void Filter::connectionCreation()
{
	connect(this,	&Filter::dataSetShouldRefresh,	_data,	&DataSet::refresh			);
	connect(this,	&Filter::refreshAllAnalyses,	_data,	&DataSet::refreshAllAnalyses);
	connect(this,	&Filter::refreshAllCompCols,	_data,	&DataSet::refreshAllCompCols);
	connect(_data,	&DataSet::datasetChanged,		this,	&Filter::datasetChanged		);	
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

	if(!_data->writeBatchedToDB())
	{
		db().transactionWriteBegin();
		db().filterUpdate(_id, _rFilter, _generatedFilter, _constructorJson, _constructorR, _name, _invalidated);

		incRevision();
		db().transactionWriteEnd();
	}
}

void Filter::dbUpdateErrorMsg()
{
	assert(_id != -1);
	
	if(!_data->writeBatchedToDB())
	{
		auto oldError = _errorMsg;
		db().transactionWriteBegin();
		db().filterUpdateErrorMsg(_id, _errorMsg);
		if(oldError != _errorMsg)
			emit filterErrorMsgChanged();
		
		incRevision();
		db().transactionWriteEnd();
	}
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

	if(oldRFilter			!= _rFilter)			emit rFilterChanged();
	if(oldGeneratedFilter	!= _generatedFilter)	emit generatedFilterChanged();
	if(oldConstructorJson	!= _constructorJson)	emit constructorJsonChanged();
	if(oldConstructorR		!= _constructorR)		emit constructorRChanged();
	
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

bool Filter::dbLoadResultAndError()
{
	assert(_id != -1);
	
	std::string newError		= db().filterLoadErrorMsg(_id);
	bool		changed			= newError != _errorMsg;
				_errorMsg		= newError;
				changed			= db().filterSelect(_id, _filtered) || changed;
	
	 if(changed)
		emit filteredChanged(); 
	 
	 calculateFilteredRowCount();
	 
	 return changed;
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
	else if(_revision >= db().filterGetRevision(_id))
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
		emit nameChanged();
}

void Filter::setRFilter(const std::string &rFilter)
{
	bool	wasChange	=_rFilter != rFilter;
			_rFilter	= rFilter;

	rescanForColumns();

	dbUpdate();

	if(wasChange)
	{
		emit rFilterChanged();
		setInvalidated(true);
	}
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
		emit filteredRowCountChanged();
}

void Filter::setGeneratedFilter(const std::string &generatedFilter)
{
	bool	wasChange			=_generatedFilter != generatedFilter;
			_generatedFilter	= generatedFilter;

	dbUpdate();

	if(wasChange)
	{
		setInvalidated(true);
		emit generatedFilterChanged();
	}
}


void Filter::setConstructorJson(const std::string &constructorJson)	
{ 
	bool	wasChange					=_constructorJson != constructorJson;
			_constructorJson			= constructorJson;

	rescanForColumns();

	dbUpdate(); 

	if(wasChange)
	{
		setInvalidated(true);
		emit constructorJsonChanged();
	}
}

void Filter::setConstructorR(const std::string &constructorR)
{
	bool	wasChange		=_constructorR != constructorR;
			_constructorR	= constructorR;

	dbUpdate();

	if(wasChange)
	{
		setInvalidated(true);
		emit constructorRChanged();
	}
}

void Filter::setInvalidated(bool invalidated)
{
	bool	wasChange		=_invalidated != invalidated;
			_invalidated	= invalidated;

	dbUpdate();

	if(wasChange)
		emit invalidatedChanged();
	
	if(_invalidated)
		_data->sendFilter(generatedFilterQ(), rFilterQ());

}

void Filter::setErrorMsg(const std::string &errorMsg)
{
	bool	wasChange	= _errorMsg != errorMsg;
			_errorMsg	= errorMsg;

	dbUpdateErrorMsg();

	if(wasChange)
		emit filterErrorMsgChanged();
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

void Filter::datasetChanged(QStringList changedColumns, QStringList missingColumns, QMap<QString, QString> changeNameColumns, bool rowCountChanged, bool)
{
	bool invalidateMe = rowCountChanged;

	if(!invalidateMe)
		for(const QString & changed : changedColumns)
			if(_columnsUsedInRFilter.count(fq(changed)) > 0 || _columnsInConstructorJson.count(fq(changed)) > 0)
			{
				invalidateMe = true;
				break;
			}

	auto iUseOneOfTheseColumns = [&](std::vector<std::string> cols) -> bool
	{
		for(const std::string & col : cols)
			if(_columnsUsedInRFilter.count(col) > 0 || _columnsInConstructorJson.count(col) > 0)
				return true;

		return false;
	};

	if(iUseOneOfTheseColumns(fq(changeNameColumns.keys())))
	{
		std::map<std::string, std::string> stdChangeNameCols(fq(changeNameColumns));

		invalidateMe = true;

		setRFilter(			ColumnEncoder::replaceColumnNamesInRScript(rFilter(),							stdChangeNameCols));
		setConstructorJson( JsonUtilities::replaceColumnNamesInDragNDropFilterJSONStr(constructorR(),		stdChangeNameCols));
	}

	auto missingStd = fq(missingColumns);
	if(iUseOneOfTheseColumns(missingStd))
	{
		setRFilter(ColumnEncoder::removeColumnNamesFromRScript(rFilter(), missingStd));

		setConstructorJson( JsonUtilities::removeColumnsFromDragNDropFilterJSONStr( constructorJson(), missingStd));

		invalidateMe = false; //Actually, if stuff is removed from the filter it won't work will it now?

		//Just reset the filter result to everything true while the user gets the change to fix their now broken filter
		reset();

		emit refreshAllAnalyses();
		data()->resetFilterCounters();
		updateStatusBar();

		//The following errormsg is overwritten immediately but that is because constructorJson changed triggers qml which triggers (some vents later) a send event. So yeah...
		//Ill leave it here though because it would be nice to show this friendlier msg then "null not found"
		setFilterErrorMsgQ(tr("Some columns were removed from the data and your filter(s)!"));
	}

	if(invalidateMe)
		setInvalidated(true);
}

int Filter::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : filtered().size();
}

int Filter::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : 1;
}

QVariant Filter::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
		return QVariant();
	
	
	if(index.row() >= rowCount() || index.column() >= columnCount())
		return QVariant(); // if there is no data then it doesn't matter what role we play
	
	switch(role)
	{
	case Qt::DisplayRole:									
	case int(dataPkgRoles::filter):							return QVariant(filtered()[index.row()]);
	}
	
	return QVariant();
}

QString Filter::constructorRQ() const
{
	return tq(constructorR());
}

QString Filter::rFilterQ() const
{
	return tq(rFilter());
}


QString Filter::nameQ() const
{
	return tq(name());
}

QString Filter::filterErrorMsgQ() const
{
	return tq(errorMsg());
}

QString Filter::generatedFilterQ() const
{
	return tq(generatedFilter());
}

QString Filter::constructorJsonQ() const
{
	return tq(constructorJson());
}

bool Filter::columnUsed(const QString &name) const
{
	return _columnsInConstructorJson.count(fq(name)) || _columnsUsedInRFilter.count(fq(name));
}

const QString & Filter::defaultRFilter()
{
	static QString defaultFilter;

	const QString forceTranslatedStuffToAlwaysBeAComment =
		tr(
			"Above you see the code that JASP generates for both value filtering and the drag&drop filter."					"\n"
			"This default result is stored in 'generatedFilter' and can be replaced or combined with a custom filter."		"\n"
			"To combine you can append clauses using '&': 'generatedFilter & customFilter & perhapsAnotherFilter'"			"\n"
			"Click the (i) icon in the lower right corner for further help."												"\n");

	defaultFilter = "# " + tq(stringUtils::replaceBy(fq(forceTranslatedStuffToAlwaysBeAComment), "\n", "\n# ") + "\n\ngeneratedFilter");

	return defaultFilter;
}

bool Filter::hasFilter() const
{
	return rFilter() != defaultRFilter() || constructorJson() != DEFAULT_FILTER_JSON; 
}

void Filter::setRFilterQ(const QString &newRFilter) 
{
	setRFilter(			fq(newRFilter));			
}

void Filter::setConstructorRQ(const QString &newConstructorR) 
{ 
	setConstructorR(	fq(newConstructorR));	
}

void Filter::setGeneratedFilterQ(const QString &newGeneratedFilter) 
{ 
	setGeneratedFilter(	fq(newGeneratedFilter));	
}

void Filter::setConstructorJsonQ(const QString &newconstructorJson) 
{ 
	setConstructorJson(		fq(newconstructorJson));	
}

void Filter::setFilterErrorMsgQ(const QString &newFilterErrorMsg) 
{ 
	setErrorMsg(fq(newFilterErrorMsg));		
}

void Filter::setStatusBarText(const QString &newStatusBarText)
{
	_statusBarText  = newStatusBarText;
}

void Filter::checkFilterResults()
{
	//Load new filter values from database
	if(dbLoadResultAndError())
	{
		emit filterErrorMsgChanged();
		emit refreshAllAnalyses();
		emit refreshAllCompCols();
		data()->resetFilterCounters(); //Should really be part of filter
		updateStatusBar();
		emit dataSetShouldRefresh();
	}
}
