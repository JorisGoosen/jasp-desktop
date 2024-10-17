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
	
	Rectangle
	{
		id:						tabView
		visible:				filterModel.filterNames
		anchors.fill:			parent
		color:					jaspTheme.uiBackground

		property var	currentTabButton
		// Be careful with https://bugreports.qt.io/browse/QTBUG-129500: the currentTabButton is set with onCheckedChanged but the tabView is not yet initialized, so the mapToItem may crash
		// As workaround the index is used: during the initialization, the first tabButton is checked, and its x is anyway 0.
		property real	currentTabX:		currentTabButton && currentTabButton.index > 0 ? currentTabButton.mapToItem(tabView, 0, 0).x : 0
		property real	currentTabWidth:	currentTabButton ? currentTabButton.width : 0

		QTC.TabBar
		{
			id:						tabbar
			contentHeight:			tabBarHeight + tabButtonRadius
			width:					parent.width - closeButton.width
			background:				Rectangle { color: jaspTheme.uiBackground }

			property real	tabBarHeight:		28 * preferencesModel.uiScale
			property real	tabButtonRadius:	5 * preferencesModel.uiScale

			Repeater
			{
				id:		tabButtonRepeater
				model:	columnModel.tabs

				QTC.TabButton
				{
					id:			tabButton
					height:		tabbar.height
					width:		labelText.implicitWidth + 20 * preferencesModel.uiScale
					
					required property int index

					onCheckedChanged:	if (checked)
											tabView.currentTabButton				= tabButton;

					background: Rectangle
					{
						color:			checked ? jaspTheme.uiBackground : jaspTheme.grayLighter
						radius:			tabbar.tabButtonRadius
						border.width:	1
						border.color:	checked ? jaspTheme.uiBorder : jaspTheme.borderColor
					}

					contentItem: Text
					{
						// The bottom of buttons are hidden to remove their bottom line with the radius
						// So the text has to be moved higher from the horizontal middle line.
						id:						labelText
						topPadding:				-tabbar.tabButtonRadius * 3/4
						text:					columnModel.tabs[index].title
						font:					jaspTheme.font
						color:					jaspTheme.black
						horizontalAlignment:	Text.AlignHCenter
						verticalAlignment:		Text.AlignVCenter
						opacity:				checked ? 1 : .6
					}

					MouseArea
					{
						anchors.fill	: parent
						cursorShape		: checked ? Qt.ArrowCursor : Qt.PointingHandCursor
						acceptedButtons	: Qt.NoButton
					}
				}
			}
		}
		
		MenuButton
		{
			id:					closeButton
			height:				33 * jaspTheme.uiScale
			width:				columnModel.compactMode ? height : 0
			iconSource:			jaspTheme.iconPath + "close-button.png"
			onClicked:			{ computedColumnWindow.askIfChangedOrClose(); columnModel.visible = false }
			toolTip:			qsTr("Close variable window")
			radius:				height
			visible:			columnModel.compactMode
			
			anchors.top:		tabView.top
			anchors.topMargin:	jaspTheme.generalAnchorMargin * -0.5
			anchors.left:		tabbar.right
			
		}

		Rectangle
		{
			// This hides the bottom border of the buttons (with their radius)
			id:			roundingHider
			width: tabbar.contentWidth
			height: tabbar.tabButtonRadius + 1
			anchors
			{
				left:		parent.left
				top:		parent.top
				topMargin:	tabbar.tabBarHeight
			}
			color: jaspTheme.uiBackground
			z: 1

			Rectangle
			{
				// The Tabbar removes the left border. Redraw it.
				anchors.left:	parent.left
				anchors.top:	parent.top
				anchors.bottom: parent.bottom
				width:			1
				color:			jaspTheme.uiBorder
			}
		}

		Rectangle
		{
			// Rectangle to draw the border under the tabbar
			id: borderView
			anchors
			{
				fill:		parent
				topMargin:	tabbar.tabBarHeight
			}
			z:				1
			border.width:	1
			border.color:	jaspTheme.uiBorder
			color:			"transparent"
		}

		Rectangle
		{
			// Hide the line onder the active tab
			z:				1
			height:			1
			width:			tabView.currentTabWidth - 2
			color:			jaspTheme.uiBackground
			x:				tabView.currentTabX + 1
			anchors.top:	borderView.top
		}

		StackLayout
		{
			id:					stack
			currentIndex:		tabbar.currentIndex >= 0 ? componentIndex[columnModel.tabs[tabbar.currentIndex].name] : -1
			clip:				true
			
			property var componentIndex:
			{
				"computed":			0,
				"label" :			1,
				"missingValues" :	2,
				"basicInfo":		3
			}
			

			anchors
			{

				top:		tabbar.bottom
				bottom:		parent.bottom
				left:		parent.left
				right:		parent.right
				margins:	jaspTheme.generalAnchorMargin * 0.5
				topMargin:	jaspTheme.generalAnchorMargin * 0.25
			}

			ComputeColumnWindow
			{
				id: computedColumnWindow
			}

			LabelEditorWindow
			{
				id: labelEditonWindow
			}

			Rectangle
			{
				id:			missingValuesView
				color:		jaspTheme.uiBackground
				enabled:	!columnModel.isVirtual

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
			}
		}
	}
}


