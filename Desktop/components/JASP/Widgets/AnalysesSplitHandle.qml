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

import QtQuick
import JASP.Widgets
import JASP.Controls
import QtQuick.Controls

Item
{
	width:				implicitWidth
	implicitWidth:		!mainWindow.analysesAvailable ? splitHandleLeft.width : analysesItem.x + analysesItem.width

	onImplicitWidthChanged:	width = implicitWidth;

	JASPSplitHandle
	{
		id:				splitHandleLeft
		z:				1000
		onArrowClicked:	mainWindow.dataPanelVisible = !mainWindow.dataPanelVisible
		pointingLeft:	mainWindow.dataPanelVisible
		showArrow:		mainWindow.dataAvailable
		toolTipArrow:	mainWindow.dataAvailable	? (mainWindow.dataPanelVisible ? qsTr("Hide data")  : qsTr("Show data")) : ""
		toolTipDrag:	mainWindow.dataPanelVisible ? qsTr("Resize data/results") : qsTr("Drag to show data")
		dragEnabled:	mainWindow.dataAvailable && mainWindow.analysesAvailable
	}

	Item
	{
		id:						analysesItem
		visible:				mainWindow.analysesAvailable
		z:						500
		width:					implicitWidth
		clip:					true
		implicitWidth:			analyses.x + analyses.implicitWidth + splitHandleRight.implicitWidth

		anchors.top:			parent.top
		anchors.bottom:			parent.bottom
		anchors.left:			splitHandleLeft.right

		AnalysisForms
		{
			id:						analyses
			x:						-implicitWidth
			width:					implicitWidth
			anchors.top:			parent.top
			anchors.bottom:			parent.bottom
			beingDragged:			splitHandleRight.hoverMouseArea.drag.active

			property int dragStartX: 0

			onXChanged:
				if(beingDragged)
				{
					if(x <= splitHandleRight.hoverMouseArea.drag.minimumX && x < analyses.dragStartX)
					{
						//dragged left and is now minimum size:
						analysesModel.visible = false;
						//That should cancel the drag as well? If it was visible anyway


					}
					/*else if(x >= splitHandleRight.hoverMouseArea.drag.maximumX && x > analyses.dragStartX)
					{
						//dragged right and now at maximum
						analysesModel.visible = true; //I guess it already was?
					}*/
				}



			property int dragMouseStart: 0



			Connections
			{
				target: analysesModel

				function onVisibleChanged(visible)
				{
					if(!visible)	analyses.x = -analyses.implicitWidth
					else			analyses.x = 0
				}
			}
		}

		JASPSplitHandle
		{
			id:						splitHandleRight
			visible:				mainWindow.analysesAvailable
			z:						1001
			showArrow:				true
			pointingLeft:			analysesModel.visible
			onArrowClicked:			analysesModel.visible = !analysesModel.visible
			toolTipDrag:			mainWindow.dataAvailable	? (mainWindow.dataPanelVisible ? qsTr("Resize data/results")  : qsTr("Drag to show data")) : ""
			toolTipArrow:			analysesModel.visible		? qsTr("Hide input options") : qsTr("Show input options")
			dragEnabled:			mainWindow.analysesAvailable

			anchors.top:			parent.top
			anchors.bottom:			parent.bottom
			anchors.left:			analyses.right

			hoverMouseArea.acceptedButtons: analysesModel.visible ? Qt.LeftButton : Qt.NoButton
			hoverMouseArea.drag.target:		analyses
			hoverMouseArea.drag.axis:		Drag.XAxis
			hoverMouseArea.drag.minimumX:	-analyses.implicitWidth
			hoverMouseArea.drag.maximumX:	0


			hoverMouseArea.onPressed: function(mouse)
			{
				analyses.dragStartX = analyses.x
				analyses.dragMouseStart = mouse.x
			}

		}
	}


}
