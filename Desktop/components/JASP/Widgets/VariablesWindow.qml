//
// Copyright (C) 2013-2023 University of Amsterdam
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

import QtQuick
import JASP
import JASP.Widgets
import JASP.Controls
import QtQuick.Controls as QTC
import QtQuick.Layouts
import "."
import "./FileMenu"


FocusScope
{
	id:			variablesContainer
	visible:	columnModel.visible

	property real calculatedBaseHeight:			(columnInfoTop.height + jaspTheme.generalAnchorMargin * 2)
	property real calculatedMinimumHeight:		calculatedBaseHeight * 1.5
	property real calculatedPreferredHeight:	Math.max(parent.height / 2, calculatedBaseHeight * 4)
	property real calculatedMaximumHeight:		!tabView.visible ? calculatedBaseHeight :  0.90 * parent.height

	Connections
	{
		target: columnModel

		function onBeforeChangingColumn(chosenColumn)
		{
			if (columnModel.visible && columnModel.chosenColumn >= 0)
			{
				columnModel.columnName			= (columnModel.compactMode ? tabInfo : columnInfoTop).columnNameValue
				columnModel.columnTitle			= (columnModel.compactMode ? tabInfo : columnInfoTop).columnTitleValue
				columnModel.columnDescription	= (columnModel.compactMode ? tabInfo : columnInfoTop).columnDescriptionValue
				columnModel.computedType		= (columnModel.compactMode ? tabInfo : columnInfoTop).columnComputedTypeValue
				columnModel.currentColumnType	= (columnModel.compactMode ? tabInfo : columnInfoTop).columnTypeValue
			}
		}
		
		function onChosenColumnChanged(chosenColumn)
		{
			if(columnModel.chosenColumn > -1 && columnModel.chosenColumn < dataSetModel.columnCount())
				//to prevent the editText in the labelcolumn to get stuck and overwrite the next columns data... We have to remove activeFocus from it
				columnInfoTop.focus = true //So we just put it somewhere
		}
	}

	Item
	{
		id:		minWidthVariables

		property int minWidth: 500 * preferencesModel.uiScale
		
		onHeightChanged: columnModel.setCompactMode(height < variablesContainer.calculatedPreferredHeight)
					
		anchors
		{
			fill:			parent
			rightMargin:	Math.min(0, variablesContainer.width - minWidth)
		}

		Rectangle
		{
			color:				jaspTheme.uiBackground
			border.color:		jaspTheme.uiBorder
			border.width:		1
			anchors.fill:		parent
			z:					-1
		}

		ColumnBasicInfo
		{
			id:					columnInfoTop
			anchors
			{
				top:			parent.top
				left:			parent.left
				right:			parent.right
				margins:		jaspTheme.generalAnchorMargin
			}
			
			visible:			!columnModel.compactMode
		}
		
		
		TabbedLayout
		{
			id:					tabView

			visible:			columnModel.tabs.length > 0
			tabs:				columnModel.tabs
			showCloseButton:	columnModel.compactMode
			
			anchors
			{
				top:			columnModel.compactMode ? parent.top : columnInfoTop.bottom
				left:			parent.left
				right:			parent.right
				bottom:			parent.bottom
				margins:		jaspTheme.generalAnchorMargin
			}

			ComputeColumnWindow
			{
				id:						computedColumnWindow
				property string name:	"computed"
			}

			LabelEditorWindow
			{
				id:						labelEditonWindow
				property string name:	"label"
			}

			Rectangle
			{
				id:						missingValuesView
				color:					jaspTheme.uiBackground
				enabled:				!columnModel.isVirtual
				property string name:	"missingValues"

				CheckBox
				{
					id:					useCustomValues
					label:				qsTr("Use custom values")
					checked:			columnModel.useCustomEmptyValues
					onCheckedChanged:	columnModel.useCustomEmptyValues = checked
				}

				PrefsMissingValues
				{
					id:					missingValues
					height:				missingValuesView.height - y
					anchors.top:		useCustomValues.bottom
					anchors.topMargin:	jaspTheme.generalAnchorMargin
					anchors.left:		parent.left
					anchors.leftMargin:	jaspTheme.generalAnchorMargin
					enabled:			useCustomValues.checked
					showTitle:			false
					model:				columnModel
					resetButtonTooltip: qsTr("Reset missing values with the ones set in your workspace")
					splitMe:			true
				}
			}
		
			ColumnBasicInfo
			{
				id:				tabInfo
				closeIcon:		false
				property string name:	"label"
			}
		}
	}
}


