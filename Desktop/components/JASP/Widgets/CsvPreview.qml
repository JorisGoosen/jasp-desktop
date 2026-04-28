import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope
{
	id: __myRoot

	Rectangle
	{
		color:				jaspTheme.white
		anchors.fill:		parent
		anchors.margins:	jaspTheme.generalMenuMargin
		z:					-1
		border.width:		1
		border.color:		jaspTheme.uiBorder

		ColumnLayout
		{
			anchors.fill: parent
			spacing: 0

			// Header / Toolbar for delimiter selection
			Rectangle
			{
				Layout.fillWidth: true
				Layout.preferredHeight: 40
				color: jaspTheme.uiBackground

				RowLayout
				{
					anchors.fill: parent
					anchors.leftMargin: 10
					anchors.rightMargin: 10
					spacing: 10

					Label {
						text: "Select Delimiter:"
						font.bold: true
						Layout.fillWidth: true
					}

					Button {
						text: ","
						onClicked: csvPreviewModel.delimiter = ','
					}
					Button {
						text: ";"
						onClicked: csvPreviewModel.delimiter = ';'
					}
					Button {
						text: "Tab"
						onClicked: csvPreviewModel.delimiter = '\t'
					}
					Button {
						text: "|"
						onClicked: csvPreviewModel.delimiter = '|'
					}
					
					Button {
						text: "Close"
						onClicked: csvPreviewModel.visible = false
						Layout.leftMargin: 20
					}
				}
			}

			// Data Preview
			TableView
			{
				id:						dataTableView
				focus:					__myRoot.focus
				Layout.fillWidth:		true
				Layout.fillHeight:		true
				
				model:					csvPreviewModel
				delegate:				Text
				{ 
					text:				modelData 
					color:				jaspTheme.textEnabled
					font:				jaspTheme.font
				}
			}
		}
	}
}
