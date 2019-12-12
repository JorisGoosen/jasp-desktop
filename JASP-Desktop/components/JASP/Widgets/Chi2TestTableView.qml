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


import QtQuick 2.0
import QtQuick.Layouts 1.11
import JASP.Controls 1.0

Item
{
	id				: chi2TestTableView
	width			: parent.width
	implicitWidth	: width
	height			: implicitHeight
	implicitHeight	: 200 * preferencesModel.uiScale

	property	alias	name				: tableView.name
	property	alias	source				: tableView.source
	property	alias	tableView			: tableView
	property	alias	showAddButton		: addButton.visible
	property	alias	showDeleteButton	: deleteButton.visible
	property	string	tableType			: "ExpectedProportions"
	property	string	itemType			: "double"
	property	int		maxNumHypotheses	: 10


	TableView
	{
		id				: tableView
		width			: chi2TestTableView.width * 3 / 4 - buttonColumn.anchors.leftMargin
		height			: chi2TestTableView.height
		modelType		: "MultinomialChi2Model"
		itemType		: chi2TestTableView.itemType
		tableType		: chi2TestTableView.tableType
	}

	Column
	{
		id					: buttonColumn
		anchors.left		: tableView.right
		anchors.leftMargin	: jaspTheme.generalAnchorMargin
		width				: chi2TestTableView.width * 1 / 4
		height				: chi2TestTableView.height
		spacing				: jaspTheme.columnGroupSpacing

		RectangularButton
		{
			id				: addButton
			text			: qsTr("Add Column")
			//name			: "addButton"
			width			: chi2TestTableView.width * 1 / 4
			onClicked		: tableView.addColumn()
			enabled			: (tableView.columnCount > 0 && tableView.columnCount < maxNumHypotheses)
		}

		RectangularButton
		{
			id				: deleteButton
			text			: qsTr("Delete Column")
			//name			: "deleteButton"
			width			: chi2TestTableView.width * 1 / 4
			onClicked		: tableView.removeAColumn()
			enabled			: tableView.columnCount > 1
		}

		RectangularButton
		{
			text			: qsTr("Reset")
			//name			: "resetButton"
			width			: chi2TestTableView.width * 1 / 4
			onClicked		: tableView.reset()
			enabled			: tableView.columnCount > 0
		}
	}

}
