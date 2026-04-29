import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JASP.Controls as JC
import JASP.Widgets  as JW

FocusScope
{
	id: __myRoot

	Rectangle
	{
		color:				jaspTheme.white
		anchors.fill:		parent
		
		z:					-1
		border.width:		1
		border.color:		jaspTheme.uiBorder
		radius:				jaspTheme.borderRadius

		ColumnLayout
		{
			anchors.fill:		parent
			anchors.margins:	jaspTheme.generalMenuMargin
			spacing:			0

			// Header / Toolbar for delimiter selection
			Rectangle
			{
				Layout.fillWidth:			true
				Layout.preferredHeight:		40 * jaspTheme.uiScale
				color:						jaspTheme.uiBackground

				RowLayout
				{
					anchors.fill:			parent
					anchors.leftMargin:		jaspTheme.generalMenuMargin
					anchors.rightMargin:	jaspTheme.generalMenuMargin
					spacing:				jaspTheme.rowSpacing
					
					Label {
						text:		qsTr("Select Delimiter:")
						font.bold:	true
					}
					
					Repeater
					{
						model:	[ ',', '.', ';', ':', '|', '\t', ' ' ]
					
						
						JC.RoundedButton 
						{
							text:			modelData == ' ' ? qsTr("Space") : modelData == '\t' ? qsTr("Tab") : modelData
							onClicked:		csvPreviewModel.delimiter =  modelData
							color:			/*csvPreviewModel.delimiter == modelData ? jaspTheme.itemHighlight :*/ defaultColor
							enabled:		csvPreviewModel.delimiter != modelData
							Layout.minimumWidth:	30 * jaspTheme.uiScale
						}
					}

					Item
					{
						Layout.fillWidth:	true	
					}
					
					JC.RoundedButton 
					{
						text:		qsTr("Select")
						onClicked:	csvPreviewModel.visible = false
						
					}
				}
			}

			// Data Preview
			Item
			{
				Layout.fillWidth:			true
				Layout.fillHeight:			true
				
				TableView
				{
					id:							dataTableView
					focus:						__myRoot.focus
					
					anchors.fill:				parent
					anchors.margins:			jaspTheme.generalAnchorMargin
					clip:						true
					
					model:						csvPreviewModel
					reuseItems:					false
					
					Connections
					{
						target:			csvPreviewModel
						
						function onClearTableForResize()
						{
							model = null;
							model = csvPreviewModel;
						}
					}
					
					delegate:					Rectangle
					{
						implicitHeight:			theText.contentHeight + jaspTheme.generalAnchorMargin
						implicitWidth:			theText.contentWidth  + jaspTheme.generalAnchorMargin
						
						color:					jaspTheme.white
						border.width:			1
						border.color:			jaspTheme.uiBorder
						
						Text
						{ 
							id:					theText
							text:				modelData 
							color:				jaspTheme.textEnabled
							font:				jaspTheme.font
							anchors.centerIn:	parent
						}
					}
				}
			}
			
			JW.PrefsLanguage { Layout.fillWidth:			true }
		}
	}
}
