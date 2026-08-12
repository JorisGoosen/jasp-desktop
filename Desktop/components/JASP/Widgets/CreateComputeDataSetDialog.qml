import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JASP.Controls	as JaspControls
import JASP

// Minimal entry point to create a computed dataset from R code. The currently shown dataset is
// used as the (single) input; the R code's last expression must be a data.frame.
Popup
{
	id:					createComputeDataSetDialogPopup

	width:				500 * preferencesModel.uiScale
	height:				computedDataSetDialogContent.height + 40 * preferencesModel.uiScale
	modal:				true
	closePolicy:		Popup.CloseOnEscape
	parent:				Overlay.overlay

	ColumnLayout
	{
		id:						computedDataSetDialogContent
		width:					parent.width - 40 * preferencesModel.uiScale
		anchors.horizontalCenter: parent.horizontalCenter
		spacing:				10 * preferencesModel.uiScale

		Text
		{
			text:				qsTr("Create computed dataset")
			font:				jaspTheme.fontGroupTitle
			color:				jaspTheme.textEnabled
		}

		Label
		{
			text:				qsTr("Name")
			color:				jaspTheme.textDisabled
		}

		TextField
		{
			id:					nameEdit
			placeholderText:	qsTr("e.g. derivedData")
			selectByMouse:		true
		}

		Label
		{
			text:				qsTr("R code (produces a data.frame)")
			color:				jaspTheme.textDisabled
		}

		TextArea
		{
			id:					rCodeEdit
			Layout.preferredHeight: 120 * preferencesModel.uiScale
			placeholderText:	qsTr("data.frame(x = someColumn * 2, y = anotherColumn)")
			selectByMouse:		true
			font:				jaspTheme.fontRCode
		}

		RowLayout
		{
			Layout.alignment:	Qt.AlignRight

			JaspControls.RectangularButton
			{
				text:			qsTr("Cancel")
				onClicked:		createComputeDataSetDialogPopup.close()
			}

			JaspControls.RectangularButton
			{
				text:			qsTr("Create")
				enabled:		nameEdit.text.length > 0 && rCodeEdit.text.length > 0
				onClicked:
				{
					workspace.createComputedDataSet(nameEdit.text, workspace.shownDataSetId, rCodeEdit.text)
					createComputeDataSetDialogPopup.close()
				}
			}
		}
	}
}