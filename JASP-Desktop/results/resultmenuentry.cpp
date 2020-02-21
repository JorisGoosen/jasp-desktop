//
// Copyright (C) 2013-2018 University of Amsterdam
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

#include "resultmenuentry.h"
#include "qquick/jasptheme.h"

//the window.blabla() functions are defined main.js and the hasBlabla are defined jaspwidgets.js
std::map<QString, ResultMenuEntry> ResultMenuEntry::AllResultEntries() //Is now a function to make sure it can get to translations, not very efficient but I do not think anyone using it will notice.
{
	return
	{
		{	"hasCopy",					ResultMenuEntry(tr("Copy"),					"hasCopy",					"copy.png",					"window.copyMenuClicked();")		},
		{	"hasRemove",				ResultMenuEntry(tr("Remove"),				"hasRemove",				"close-button.png",			"window.removeMenuClicked();")		},
		{	"hasNotes",					ResultMenuEntry(tr("Add Note"),				"hasNotes",					"",							"")									},
		{	"hasCollapse",				ResultMenuEntry(tr("Collapse"),				"hasCollapse",				"collapse.png",				"window.collapseMenuClicked();")	},
		{	"hasDuplicate",				ResultMenuEntry(tr("Duplicate"),			"hasDuplicate",				"duplicate.png",			"window.duplicateMenuClicked();")	},
		{	"hasEditTitle",				ResultMenuEntry(tr("Edit Title"),			"hasEditTitle",				"edit-pencil.png",			"window.editTitleMenuClicked();")	},
		{	"hasEditImg",				ResultMenuEntry(tr("Edit Image"),			"hasEditImage",				"editImage.png",			"window.editImageClicked();")		},
		{	"hasRemoveAllAnalyses",		ResultMenuEntry(tr("Remove All"),			"hasRemoveAllAnalyses",		"close-button.png",			"")									},
		{	"hasLaTeXCode",				ResultMenuEntry(tr("Copy LaTeX"),			"hasLaTeXCode",				"code-icon.png",			"window.latexCodeMenuClicked();")	},
		{	"hasRefreshAllAnalyses",	ResultMenuEntry(tr("Refresh All"),			"hasRefreshAllAnalyses",	"",							"")									},
		{	"hasSaveImg",				ResultMenuEntry(tr("Save Image As"),		"hasSaveImg",				"document-save-as.png",		"window.saveImageClicked();")		},
		{	"hasCite",					ResultMenuEntry(tr("Copy Citations"),		"hasCite",					"cite.png",					"window.citeMenuClicked();")		},
		{	"hasShowDeps",				ResultMenuEntry(tr("Show Dependencies"),	"hasShowDeps",				"",							"window.showDependenciesClicked()")	}
	};
}

QStringList ResultMenuEntry::EntriesOrder = {"hasCollapse", "hasEditTitle", "hasCopy", "hasLaTeXCode", "hasCite", "hasSaveImg",
											 "hasEditImg", "hasNotes", "hasDuplicate", "hasRemove", "hasRemoveAllAnalyses", "hasRefreshAllAnalyses", "hasShowDeps"};

ResultMenuEntry::ResultMenuEntry(QString displayText, QString name, QString menuImageSource, QString jsFunction)
	: _displayText(displayText)
	, _name(name)
	, _menuImageSource(menuImageSource)
	, _jsFunction(jsFunction)
	, _isSeparator(false)
	, _isEnabled(true)
{
}

ResultMenuEntry::ResultMenuEntry()
{
	_isSeparator = true;
}

QString	ResultMenuEntry::menuImageSource() const
{

	return _menuImageSource == "" ? "" : JaspTheme::currentIconPath() + _menuImageSource;
}
