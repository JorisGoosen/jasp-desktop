#include "dataset.h"
#include "log.h"
#include <regex>

DataSet::DataSet(int index)
	: DataSetBaseNode(dataSetBaseNodeType::dataSet, nullptr)
{
	_dataNode		= new DataSetBaseNode(dataSetBaseNodeType::data,	this);
	_filtersNode	= new DataSetBaseNode(dataSetBaseNodeType::filters, this);
	
	if(index == -1)	dbCreate();
	else			dbLoad(index);
}

DataSet::~DataSet()
{
	//delete columns before dataNode as they depend on it via DataSetBaseNode inheritance
	for(Column * col : _columns)
		delete col;

	_columns.clear();

	delete _dataNode;
	_dataNode = nullptr;
	
	delete _filter;
	_filter = nullptr;
}

void DataSet::dbDelete()
{
	assert(_dataSetID != -1);

	db().transactionWriteBegin();

	if(_filter && _filter->id() != -1)
		_filter->dbDelete();
	_filter = nullptr;

	for(Column * col : _columns)
		col->dbDelete();

	db().dataSetDelete(_dataSetID);

	_dataSetID = -1;

	db().transactionWriteEnd();
}

void DataSet::beginBatchedToDB()
{
	assert(!_writeBatchedToDB);
	_writeBatchedToDB = true;
}

void DataSet::endBatchedToDB()
{
	assert(_writeBatchedToDB);
	_writeBatchedToDB = false;

	db().dataSetBatchedValuesUpdate(this);
	incRevision(); //Should trigger reload at engine end
}

int DataSet::getColumnIndex(const std::string & name) const 
{
	for(size_t i=0; i<_columns.size(); i++)
		if(_columns[i]->name() == name)
			return i;
	return -1;
}

int DataSet::columnIndex(const Column * col) const
{
	for(size_t i=0; i<_columns.size(); i++)
		if(_columns[i] == col)
			return i;
	return -1;
}

Column *DataSet::column(const std::string &name)
{
	for(Column * column : _columns)
		if(column->name() == name)
			return column;

	return nullptr;
}

Column *DataSet::column(size_t index)
{
	if(index >= _columns.size())
		return nullptr;

	return _columns[index];
}

void DataSet::removeColumn(size_t index)
{
	assert(_dataSetID > 0);

	Column * removeMe = _columns[index];
	_columns.erase(_columns.begin() + index);

	removeMe->dbDelete();
	delete removeMe;

	incRevision();
}

void DataSet::removeColumn(const std::string & name)
{
	assert(_dataSetID > 0);

	for(auto col = _columns.begin() ; col != _columns.end(); col++)
		if((*col)->name() == name)
		{
			(*col)->dbDelete();
			_columns.erase(col);

			incRevision();

			return;
		}
}

void DataSet::insertColumn(size_t index)
{

	assert(_dataSetID > 0);
	_columns.insert(_columns.begin()+index, new Column(this, db().columnInsert(_dataSetID, index)));

	incRevision();
}

Column * DataSet::newColumn(const std::string &name)
{
	assert(_dataSetID > 0);
	Column * col = new Column(this, db().columnInsert(_dataSetID, -1, name));
	_columns.push_back(col);

	incRevision();

	return col;
}

size_t DataSet::getMaximumColumnWidthInCharacters(size_t columnIndex) const
{
	if(columnIndex >= columnCount())
		return 0;

	const Column * col = _columns[columnIndex];

	int extraPad = 4;

	switch(col->type())
	{
	case columnType::scale:
		return 9 + extraPad; //default precision of stringstream is 6 (and sstream is used in displaying scale values) + 3 because Im seeing some weird stuff with exp-notation  etc + some padding because of dots and whatnot

	case columnType::unknown:
		return 0;

	default:
	{
		int tempVal = 0;

		for(Label * label : col->labels())
			tempVal = std::max(tempVal, static_cast<int>(label->label(true).length()));

		return tempVal + extraPad;
	}
	}

}

stringvec DataSet::getColumnNames()
{
	stringvec names;

	for(Column * col : _columns)
		names.push_back(col->name());

	return names;
}

void DataSet::dbCreate()
{
	assert(!_filter && _dataSetID == -1);

	db().transactionWriteBegin();

	//The variables are probably empty though:
	_dataSetID	= db().dataSetInsert(_dataFilePath, _emptyValues.toJson().toStyledString(), _databaseJson);	
	_filter = new Filter(this);
	_filter->dbCreate();
	_columns.clear();

	db().transactionWriteEnd();

	_rowCount		= 0;
}

void DataSet::dbUpdate()
{
	assert(_dataSetID > 0);
	db().dataSetUpdate(_dataSetID, _dataFilePath, _emptyValues.toJson().toStyledString(), _databaseJson);
	incRevision();
}

void DataSet::dbLoad(int index)
{
	Log::log() << "loadDataSet(index=" << index << "), _dataSetID="<< _dataSetID <<";" << std::endl;

	assert(_dataSetID == -1 || _dataSetID == index || (_dataSetID != -1 && index == -1));

	if(index != -1 && !db().dataSetExists(index))
	{
		Log::log() << "No such DataSet!" << std::endl;
		return;
	}
	
	if(index != -1)
		_dataSetID	= index;

	assert(_dataSetID > 0);

	std::string emptyVals;

	db().dataSetLoad(_dataSetID, _dataFilePath, emptyVals, _databaseJson, _revision);

	if(!_filter)
		_filter = new Filter(this);
	_filter->dbLoad();

	int colCount = db().dataSetColCount(_dataSetID);


	_rowCount		= db().dataSetRowCount(_dataSetID);
	Log::log() << "colCount: " << colCount << ", " << "rowCount: " << rowCount() << std::endl;
			
	for(size_t i=0; i<colCount; i++)
	{
		if(_columns.size() == i)
			_columns.push_back(new Column(this));

		_columns[i]->dbLoadIndex(i, false);
	}

	for(size_t i=colCount; i<_columns.size(); i++)
		delete _columns[i];

	_columns.resize(colCount);

	db().dataSetBatchedValuesLoad(this);

	Json::Value emptyValsJson;
	std::stringstream(emptyVals) >> emptyValsJson;
	_emptyValues.fromJson(emptyValsJson);
}

int DataSet::columnCount() const
{
	return _columns.size();
}

int DataSet::rowCount() const
{
	return _rowCount;
}

void DataSet::setColumnCount(size_t colCount)
{
	db().transactionWriteBegin();

	int curCount = columns().size();

	if(colCount > curCount)
		for(size_t i=curCount; i<colCount; i++)
			insertColumn(i);

	else if(colCount < curCount)
		for(size_t i=curCount-1; i>=colCount; i--)
			removeColumn(i);
	
	incRevision();

	db().transactionWriteEnd();
}

void DataSet::setRowCount(size_t rowCount)
{
	_rowCount = rowCount; //Make sure we do set the rowCount variable here so the batch can easily see how big it ought to be in DatabaseInterface::dataSetBatchedValuesUpdate

	if(!writeBatchedToDB())
	{
		db().dataSetSetRowCount(_dataSetID, rowCount);
		dbLoad(); //Make sure columns have the right data in them
	}

	_filter->reset();
}

void DataSet::incRevision()
{
	assert(_dataSetID != -1);

	if(!writeBatchedToDB())
	{
		_revision = db().dataSetIncRevision(_dataSetID);
		checkForChanges();
	}
}

void DataSet::checkForUpdates()
{
	JASPTIMER_SCOPE(DataSet::checkForUpdates);

	if(_dataSetID == -1)
		return;

	if(_revision != db().dataSetGetRevision(_dataSetID))
		dbLoad();
	else
	{
		_filter->checkForUpdates();

		for(Column * col : _columns)
			col->checkForUpdates();
	}
}

const Columns & DataSet::computedColumns() const
{
	static Columns computedColumns;

	computedColumns.clear();

	for(Column * column : _columns)
		if(column->isComputed())
			computedColumns.push_back(column);

	return computedColumns;
}

void DataSet::loadOldComputedColumnsJson(const Json::Value &json)
{
	for(const Json::Value & colJson : json)
	{
		const std::string name = json["name"].asString();

		Column * col = column(name);

		if(!col && !name.empty())
			col = newColumn(name);

		if(!col)
			continue;

		col->loadComputedColumnJsonBackwardsCompatibly(colJson);
	}

	for(Column * col : computedColumns())
		col->findDependencies();
}

std::map<std::string, intstrmap> DataSet::resetEmptyValues()
{
	std::map<std::string, intstrmap> colChanged;

	for (Column * col : _columns)
	{
		intstrmap emptyValuesMap;

		if (_emptyValues._map.count(col->name()))
			emptyValuesMap = _emptyValues._map.at(col->name());

		if (col->resetEmptyValues(emptyValuesMap))
			colChanged[col->name()] = emptyValuesMap;
	}

	incRevision();

	return colChanged;
}

stringset DataSet::findUsedColumnNames(std::string searchThis)
{
	//sort of based on rbridge_encodeColumnNamesToBase64
	static std::regex nonNameChar("[^\\.A-Za-z0-9]");
	std::set<std::string> columnsFound;
	size_t foundPos = -1;

	for(Column * column : _columns)
	{
		const std::string & col = column->name();
		
		while((foundPos = searchThis.find(col, foundPos + 1)) != std::string::npos)
		{
			size_t foundPosEnd = foundPos + col.length();
			//First check if it is a "free columnname" aka is there some space or a kind in front of it. We would not want to replace a part of another term (Imagine what happens when you use a columname such as "E" and a filter that includes the term TRUE, it does not end well..)
			bool startIsFree	= foundPos == 0							|| std::regex_match(searchThis.substr(foundPos - 1, 1),	nonNameChar);
			bool endIsFree		= foundPosEnd == searchThis.length()	|| (std::regex_match(searchThis.substr(foundPosEnd, 1),	nonNameChar) && searchThis[foundPosEnd] != '('); //Check for "(" as well because maybe someone has a columnname such as rep or if or something weird like that

			if(startIsFree && endIsFree)
			{
				columnsFound.insert(col);
				searchThis.replace(foundPos, col.length(), ""); // remove the found entry
			}

		}
	}

	return columnsFound;
}
