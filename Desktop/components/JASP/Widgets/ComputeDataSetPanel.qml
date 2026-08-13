import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JASP.Controls	as JaspControls
import JASP

// Bottom-of-datapanel editor for computed datasets. Only visible when the shown
// dataset is a computed (R) dataset. The R code's last expression must be a
// data.frame; it is written back into the output dataset via .setDataSet.
Rectangle
{
	id:							root

	readonly property var		workspace:			dataSetPackage.workspace
	readonly property var		shownDataSet:		workspace.shownDataSet
	readonly property string	currentInputName:	shownDataSet ? workspace.dataSetNameById(shownDataSet.defaultInputDataSetId) : ""

	property bool				expanded:			true
	property var				inputNames:			[]

	property real				headerHeight:		30 * preferencesModel.uiScale
	property real				contentHeight:		220 * preferencesModel.uiScale

	height:						expanded ? headerHeight + contentHeight : headerHeight

	color:						jaspTheme.uiBackground
	border.color:				jaspTheme.grayLighter
	border.width:				1

	function refreshInputNames()
	{
		var all		= workspace.dataSetNames()
		var shown	= workspace.shownDataSet ? workspace.shownDataSet.name : ""
		var filtered	= []
		for(var i = 0; i < all.length; i++)
			if(all[i] !== shown)
				filtered.push(all[i])
		inputNames = filtered
	}

	function syncFromShown()
	{
		refreshInputNames()
		if(shownDataSet)
		{
			codeEdit.text = shownDataSet.rCode
			inputDropDown.currentValue = currentInputName
		}
	}

	function applyComputedDataSet()
	{
		if(!shownDataSet)
			return

		shownDataSet.rCode = codeEdit.text
	}

	Connections
	{
		target:					workspace

		function onShownDataSetChanged()
		{
			root.syncFromShown()
		}
	}

	Component.onCompleted:		syncFromShown()

	// ---------- header (always visible when computed) ----------
	RowLayout
	{
		id:						headerLayout
		anchors.left:			parent.left
		anchors.right:			parent.right
		anchors.top:			parent.top
		height:					root.headerHeight
		anchors.margins:		jaspTheme.generalAnchorMargin

		Text
		{
			Layout.fillWidth:	true
			text:				qsTr("Computed dataset")
			font:				jaspTheme.fontGroupTitle
			color:				jaspTheme.textEnabled
			verticalAlignment:	Text.AlignVCenter
		}

		Text
		{
			text:				shownDataSet ? shownDataSet.title : ""
			font:				jaspTheme.font
			color:				shownDataSet && shownDataSet.invalidated ? jaspTheme.textDisabled : jaspTheme.textEnabled
			verticalAlignment:	Text.AlignVCenter
			Layout.maximumWidth: 200 * jaspTheme.uiScale
			elide:				Text.ElideRight
		}

		JaspControls.RectangularButton
		{
			id:					toggleButton
			text:				root.expanded ? qsTr("Hide") : qsTr("Show")
			onClicked:			root.expanded = !root.expanded
		}
	}

	// ---------- content ----------
	ColumnLayout
	{
		id:						contentColumn
		visible:				root.expanded
		anchors.top:			headerLayout.bottom
		anchors.left:			parent.left
		anchors.right:			parent.right
		anchors.bottom:			parent.bottom
		anchors.margins:		jaspTheme.generalAnchorMargin
		spacing:				6 * jaspTheme.uiScale

		RowLayout
		{
			Layout.fillWidth:	true

			Text
			{
				text:			qsTr("Input dataset")
				font:			jaspTheme.font
				color:			jaspTheme.textEnabled
			}

			JaspControls.DropDown
			{
				id:				inputDropDown
				Layout.fillWidth: true
				values:			root.inputNames
				startValue:		""
				currentValue:	root.currentInputName
				onValueChanged:
				{
					if(shownDataSet && inputDropDown.currentValue.length > 0)
						shownDataSet.defaultInputDataSetId = workspace.dataSetIdByName(inputDropDown.currentValue)
				}
			}
		}

		Text
		{
			text:				qsTr("R code (produces a data.frame)")
			font:				jaspTheme.font
			color:				jaspTheme.textEnabled
		}

		Rectangle
		{
			id:					codeEditRectangle
			Layout.fillWidth:	true
			Layout.fillHeight:	true
			color:				jaspTheme.white
			border.color:		jaspTheme.grayLighter
			border.width:		1

			TextArea
			{
				id:					codeEdit
				anchors.fill:		parent
				anchors.margins:	2
				placeholderText:	qsTr("data.frame(x = someColumn * 2, y = anotherColumn)")
				selectByMouse:		true
				wrapMode:			TextArea.WrapAtWordBoundaryOrAnywhere
				color:				jaspTheme.textEnabled
				font:				jaspTheme.fontRCode

				JaspControls.RSyntaxHighlighterQuick
				{
					textDocument:	codeEdit.textDocument
				}
			}
		}

		RowLayout
		{
			Layout.fillWidth:	true

			Text
			{
				id:				errorText
				Layout.fillWidth: true
				visible:		shownDataSet && shownDataSet.error.length > 0
				text:			shownDataSet ? shownDataSet.error : ""
				color:			jaspTheme.red
				font:			jaspTheme.fontCode
				wrapMode:		Text.Wrap
			}

			JaspControls.RectangularButton
			{
				id:				applyButton
				text:			qsTr("Apply computed dataset")
				Layout.alignment: Qt.AlignRight
				onClicked:		root.applyComputedDataSet()
			}
		}
	}
}
