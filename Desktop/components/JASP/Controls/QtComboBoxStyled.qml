import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ComboBox
{
					id:						control

					focus:					true
					padding:				2 * preferencesModel.uiScale //jaspTheme.jaspControlPadding

					width:					modelWidth + extraWidth
					height:					jaspTheme.comboBoxHeight
					font:					jaspTheme.font
	property int	modelWidth:				30 * preferencesModel.uiScale
	property int	extraWidth:				5 * padding + dropdownIcon.width
	property bool	isEmptyValue:			false
	property bool	showEmptyValueStyle:	false

	property bool	showIcon:				false
	property string	iconSource:				""
	property bool	showBorder:				true
	property bool	useExternalBorder:		false
	property var	enabledOptions:			[] //A list to enable items

	property bool	addEmptyValue:			false
	property bool	showEmptyValueAsNormal:	false
	property bool	addLineAfterEmptyValue:	false
	property bool	showVariableTypeIcon:	false
	property bool	addScrollBar:			false

	property string placeholderText:		""

	TextMetrics
	{
		id:		textMetrics
		font:	control.font

		property bool initialized: false

		onWidthChanged:
		{
			if (initialized)
				_resetWidth(width)
		}
	}

	contentItem: Rectangle
	{
		color:	jaspTheme.controlBackgroundColor
		Image
		{
			id:						contentIcon
			height:					15 * preferencesModel.uiScale
			width:					15 * preferencesModel.uiScale
			x:						3  * preferencesModel.uiScale
			anchors.verticalCenter: parent.verticalCenter
			source:					iconSource
			visible:				showIcon
		}

		Text
		{
			anchors.left:				contentIcon.visible ? contentIcon.right : parent.left
			anchors.leftMargin:			4 * preferencesModel.uiScale
			anchors.verticalCenter:		parent.verticalCenter
			anchors.horizontalCenter:	control.showEmptyValueStyle ? parent.horizontalCenter : undefined
			text:						control.currentText
			font:						control.font
			color:						(!enabled || control.showEmptyValueStyle) ? jaspTheme.grayDarker : jaspTheme.black
		}
	}

	indicator: Image
	{
		id:			dropdownIcon
		x:			control.width - width - 3 //control.spacing
		y:			control.topPadding + (control.availableHeight - height) / 2
		width:		12 * preferencesModel.uiScale
		height:		12 * preferencesModel.uiScale
		source:		jaspTheme.iconPath + "/toolbutton-menu-indicator.svg"

	}

	background: Rectangle
	{
		id:				comboBoxBackground
		border.width:	showBorder && !control.activeFocus ? 1					   : 0
		border.color:	showBorder						   ? jaspTheme.borderColor : "transparent"
		radius:			2
		color:			jaspTheme.controlBackgroundColor
	}

	Rectangle
	{
		id:					externalControlBackground
		height:				parent.height + jaspTheme.jaspControlHighlightWidth
		width:				parent.width + jaspTheme.jaspControlHighlightWidth
		color:				"transparent"
		border.width:		1
		border.color:		"transparent"
		anchors.centerIn:	parent
		visible:			useExternalBorder
		radius:				jaspTheme.jaspControlHighlightWidth
	}

	popup: Popup
	{
		y:				control.height - 1
		width:			comboBoxBackground.width
		padding:		1

		enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 } enabled: preferencesModel.animationsOn }

		JASPScrollBar
		{
			id:				scrollBar
			flickable:		popupView
			manualAnchor:	true
			vertical:		true
			visible:		addScrollBar
			z:				1337

			anchors
			{
				top:		parent.top
				right:		parent.right
				bottom:		parent.bottom
				margins:	2
			}
		}


		contentItem: ListView
		{
			id:				popupView
			width:			comboBoxBackground.width
			implicitHeight: contentHeight
			model:			control.popup.visible ? control.delegateModel : null
			currentIndex:	control.highlightedIndex
			clip:			true

			Rectangle
			{
				anchors.centerIn:	parent
				width:				parent.width + 4
				height:				parent.height + 4
				border.color:		jaspTheme.focusBorderColor
				border.width:		2
				color:				"transparent"
			}
		}
	}

	delegate: ItemDelegate
	{
		height:								jaspTheme.comboBoxHeight
		width:								comboBoxBackground.width
		enabled:							enabledOptions.length == 0 || enabledOptions.length <= index || enabledOptions[index]

		contentItem: Rectangle
		{
			id:								itemRectangle
			anchors.fill:					parent
			anchors.rightMargin:			scrollBar.visible ? scrollBar.width + 2 : 0
			color:							control.currentIndex === index ? jaspTheme.itemSelectedColor : (control.highlightedIndex === index ? jaspTheme.itemHoverColor : jaspTheme.controlBackgroundColor)

			property bool isEmptyValue:			addEmptyValue && index === 0
			property bool showEmptyValueStyle:	!showEmptyValueAsNormal && isEmptyValue
			property bool showLine:				addLineAfterEmptyValue && index === 0


			Image
			{
				id:							delegateIcon
				x:							1 * preferencesModel.uiScale
				height:						15 * preferencesModel.uiScale
				width:						15 * preferencesModel.uiScale
				source:						visible ? model.columnTypeIcon : ""
				visible:					showVariableTypeIcon && !itemRectangle.isEmptyValue

				anchors.verticalCenter:		parent.verticalCenter
			}

			Text
			{
				x:							(delegateIcon.visible ? 20 : 4) * preferencesModel.uiScale
				text:						itemRectangle.isEmptyValue ? placeholderText : (model && model.name ? model.name : modelData ? modelData : "")
				font:						jaspTheme.font
				color:						itemRectangle.showEmptyValueStyle || !enabled ? jaspTheme.grayDarker : (control.currentIndex === index ? jaspTheme.white : jaspTheme.black)
				anchors.verticalCenter:		parent.verticalCenter
				anchors.horizontalCenter:	itemRectangle.showEmptyValueStyle ? parent.horizontalCenter : undefined
			}

			Rectangle
			{
				anchors
				{
					left:	parent.left
					right:	parent.right
					bottom: parent.bottom
				}
				visible:	itemRectangle.showLine
				height:		1
				color:		jaspTheme.focusBorderColor
			}
		}
	}
}
