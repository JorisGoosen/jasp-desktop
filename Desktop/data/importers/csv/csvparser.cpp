//
// Copyright (C) 2013-2026 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//
#include "csvparser.h"

#include <boost/algorithm/string.hpp>

using namespace std;

CSVParser::CSVParser(char delimiter, bool replaceLineEndings)
	: QObject()
	, _delimiter(delimiter)
	, _replaceLineEndings(replaceLineEndings)
{
	reset();
}

void CSVParser::parse(const string& data)
{
	reset();
	size_t i = 0;
	while (i < data.size()) {
		char ch = data[i];
		if (processChar(ch)) {
			// Re-process the same character
			continue;
		}
		i++;
	}
	finishRow();
	emit parsingComplete();
}

void CSVParser::parse(const QString& data)
{
	parse(data.toStdString());
}

bool CSVParser::processChar(char ch)
{
	switch (_state)
	{
	case Normal:
		if (ch == '"')
		{
			_state = Quoted;
		}
		else if (ch == _delimiter)
		{
			finishField();
		}
		else if (ch == '\r')
		{
			_currentField.push_back(ch);
		}
		else if (ch == '\n')
		{
			if (!_currentField.empty() && _currentField.back() == '\r')
			{
				_currentField.pop_back();
				_currentField += ' ';
			}
			finishField();
		_grid.push_back(_currentRow);
		_currentRow.clear();
		_rowFinished = true;
		emit rowParsed(_grid.back());
		}
		else
		{
			_currentField.push_back(ch);
		}
		return false;

	case Quoted:
		if (ch == '"')
		{
			_state = QuotedQuote;
		}
		else
		{
			_currentField.push_back(ch);
		}
		return false;

	case QuotedQuote:
		if (ch == '"')
		{
			_currentField.push_back('"');
			_state = Quoted;
		}
		else
		{
			_state = Normal;
			return true;
		}
		return false;
	}
	return false;
}

bool CSVParser::hasRow() const
{
	return !_grid.empty() || (_rowFinished && (!_currentRow.empty() || !_currentField.empty()));
}

vector<string> CSVParser::extractRow()
{
	if (_rowFinished && (!_currentRow.empty() || !_currentField.empty()))
	{
		finishRow();
	}

	if (_grid.empty())
	{
		return {};
	}

	auto row = _grid.front();
	_grid.erase(_grid.begin());
	return row;
}

const vector<vector<string>>& CSVParser::getGrid() const
{
	return _grid;
}

size_t CSVParser::getRowCount() const
{
	return _grid.size();
}

size_t CSVParser::getColumnCount(size_t row) const
{
	if (row >= _grid.size())
	{
		return 0;
	}
	return _grid[row].size();
}

bool CSVParser::hasPendingData() const
{
	return !_currentField.empty() || !_currentRow.empty() || !_grid.empty();
}

void CSVParser::reset()
{
	_state = Normal;
	_currentField.clear();
	_currentRow.clear();
	_grid.clear();
	_rowFinished = false;
}

void CSVParser::setDelimiter(char delimiter)
{
	_delimiter = delimiter;
	reset();
}

void CSVParser::finishField()
{
	if (_replaceLineEndings)
	{
		replaceLineEndings(_currentField);
	}
	_currentRow.push_back(_currentField);
	_currentField.clear();
}

void CSVParser::finishRow()
{
	if (!_currentField.empty() || !_currentRow.empty())
	{
		finishField();
	}

	if (!_currentRow.empty())
	{
		_grid.push_back(_currentRow);
		_currentRow.clear();
	}
	_rowFinished = false;
}

void CSVParser::replaceLineEndings(string& field) const
{
	boost::algorithm::replace_all(field, "\r\n", " ");
	boost::algorithm::replace_all(field, "\r", " ");
	boost::algorithm::replace_all(field, "\n", " ");
}
