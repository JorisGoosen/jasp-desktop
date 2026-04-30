import QtQuick
import QtQuick.Layouts
import JASP.Controls as JC
import JASP.Widgets

Window
{
	id: csvPreviewWindow

	minimumWidth:           600 * jaspTheme.uiScale
	minimumHeight:          600 * jaspTheme.uiScale
	visible:                csvPreviewModel.visible
	title:					qsTr("Data Preview")
	modality:               Qt.ApplicationModal
	color:                  jaspTheme.white

	property real windowPadding: 20 * jaspTheme.uiScale

	onClosing:
	{
		csvPreviewModel.delimiter = '\0'
		csvPreviewModel.visible = false
	}

	// Header / Toolbar for delimiter selection
	Rectangle
	{
		id: delimetersRect
		anchors
		{
			top:				parent.top
			left:				parent.left
			right:				parent.right
			margins:			windowPadding
		}

		height:					50 * jaspTheme.uiScale
		color:					jaspTheme.uiBackground

		Item
		{
			id:					delimetersInsideRect
			anchors.fill:		parent
			anchors.margins:	jaspTheme.generalAnchorMargin

			RowLayout
			{
				spacing:		jaspTheme.rowSpacing

				JC.Label
				{
					text:		qsTr("Select Delimiter:")
					font.bold:	true
					Layout.alignment: Qt.AlignHCenter
				}

				Repeater
				{
					model:	[ ',', '.', ';', ':', '|', '\t', ' ' ]


					JC.RoundedButton
					{
						text:			modelData == ' ' ? qsTr("Space") : modelData == '\t' ? qsTr("Tab") : modelData
						onClicked:		csvPreviewModel.delimiter =  modelData
						enabled:		csvPreviewModel.delimiter != modelData
						Layout.minimumWidth: 30 * jaspTheme.uiScale
						defaultBorderColor: enabled ? jaspTheme.buttonBorderColor : jaspTheme.black
					}
				}
			}

			JC.CheckBox
			{
				id:						advanced
				anchors.right:			parent.right
				anchors.rightMargin:	jaspTheme.generalAnchorMargin
				anchors.verticalCenter: parent.verticalCenter
				label:					qsTr("Advanced")
			}
		}
	}

	PrefsLanguage
	{
		id:					prefLanguage
		anchors.top:		delimetersRect.bottom
		anchors.left:		parent.left
		anchors.right:		parent.right
		anchors.margins:	windowPadding
		visible:			advanced.checked
		nextTabItem:		submitButton
		showHelpLink:		false
	}

	// Data Preview
	Rectangle
	{
		anchors
		{
			left:				parent.left
			right:				parent.right
			top:				prefLanguage.visible ? prefLanguage.bottom : delimetersRect.bottom
			bottom:				buttons.top
			margins:			windowPadding
		}
		color:					jaspTheme.white
		border.width:			1
		border.color:			jaspTheme.black

		JC.JASPScrollBar
		{
			id:				vertiScroller;
			flickable:		dataTableView
			anchors.top:	parent.top
			anchors.right:	parent.right
			anchors.bottom: horiScroller.top
		}

		JC.JASPScrollBar
		{
			id:				horiScroller;
			flickable:		dataTableView
			vertical:		false
			anchors.left:	parent.left
			anchors.right:	vertiScroller.left
			anchors.bottom: parent.bottom
		}

		TableView
		{
			id:							dataTableView

			anchors.top:				parent.top
			anchors.left:				parent.left
			anchors.right:				vertiScroller.left
			anchors.bottom:				horiScroller.top
			anchors.leftMargin:			1 // border line
			anchors.topMargin:			1

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

	Row
	{
		id: buttons
		spacing: 10 * jaspTheme.uiScale

		anchors.bottom:         parent.bottom
		anchors.bottomMargin:   windowPadding
		anchors.left:           parent.left
		anchors.leftMargin:     windowPadding

		property real buttonWidth: (csvPreviewWindow.width - windowPadding * 2 - buttons.spacing) / 2
		JC.Button
		{
			id: submitButton
			text: qsTr("Load")
			width: buttons.buttonWidth
			control.color: jaspTheme.blue
			onClicked: {
				csvPreviewModel.visible = false
			}
		}

		JC.Button
		{
			id: cancelButton
			text: qsTr("Cancel")
			width: buttons.buttonWidth
			onClicked: {
				csvPreviewModel.delimiter = '\0'
				csvPreviewModel.visible = false
			}
		}

	}
}
