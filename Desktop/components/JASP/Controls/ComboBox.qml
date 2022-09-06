import QtQuick			2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts	1.3
import JASP				1.0


ComboBoxBase
{
	id:					comboBox
	implicitHeight:		control.height + ((controlLabel.visible && setLabelAbove) ? rectangleLabel.height : 0)
	implicitWidth:		control.width + ((controlLabel.visible && !setLabelAbove) ? jaspTheme.labelSpacing + controlLabel.width : 0)
	background:			useExternalBorder ? externalControlBackground : control.background
	innerControl:		control
	title:				label

	property alias	control:				control
	property alias	controlLabel:			controlLabel
	property alias	label:					controlLabel.text
	property alias	currentLabel:			comboBox.currentText
	property alias	value:					comboBox.currentValue
	property alias	indexDefaultValue:		comboBox.currentIndex
	property alias	fieldWidth:				control.modelWidth
	property bool	showVariableTypeIcon:	containsVariables
	property var	enabledOptions:			[]
	property bool	setLabelAbove:			false
	property int	controlMinWidth:		0
	property bool	useExternalBorder:		true
	property bool	showBorder:				true
	property bool	addScrollBar:			false
	property bool	showEmptyValueAsNormal:	false
	property bool	addLineAfterEmptyValue:	false

	onControlMinWidthChanged: _resetWidth(textMetrics.width)

	function resetWidth(values)
	{
		var maxWidth = 0
		var maxValue = ""
		textMetrics.initialized = false;

		if (addEmptyValue)
			values.push(placeholderText)

		for (var i = 0; i < values.length; i++)
		{
			textMetrics.text = values[i]
			if (textMetrics.width > maxWidth)
			{
				maxWidth = textMetrics.width
				maxValue = values[i]
			}
		}

		textMetrics.text = maxValue;
		textMetrics.initialized = true;
		_resetWidth(maxWidth)
	}

	function _resetWidth(maxWidth)
	{
		var newWidth = maxWidth + ((comboBox.showVariableTypeIcon ? 20 : 4) * preferencesModel.uiScale);
		control.modelWidth = newWidth;
		if (control.width < controlMinWidth)
			control.modelWidth += (controlMinWidth - control.width);
		comboBox.width = comboBox.implicitWidth; // the width is not automatically updated by the implicitWidth...
    }

	Component.onCompleted:	control.activated.connect(activated);

	Rectangle
	{
		id:			rectangleLabel
		width:		controlLabel.width
		height:		control.height
		color:		debug ? jaspTheme.debugBackgroundColor : "transparent"
		visible:	controlLabel.text && comboBox.visible ? true : false
		Label
		{
			id:			controlLabel
			font:		jaspTheme.font
			anchors.verticalCenter: parent.verticalCenter
			color:		enabled ? jaspTheme.textEnabled : jaspTheme.textDisabled
		}
	}

	QtComboBoxStyled
	{
						id:				control
						model:			comboBox.model

						anchors.left:	!rectangleLabel.visible || comboBox.setLabelAbove ? comboBox.left : rectangleLabel.right
						anchors.leftMargin: !rectangleLabel.visible || comboBox.setLabelAbove ? 0 : jaspTheme.labelSpacing
						anchors.top:	rectangleLabel.visible && comboBox.setLabelAbove ? rectangleLabel.bottom: comboBox.top

						focus:			true
						padding:		2 * preferencesModel.uiScale //jaspTheme.jaspControlPadding

						width:			modelWidth + extraWidth
						height:			jaspTheme.comboBoxHeight
						font:			jaspTheme.font
		property int	modelWidth:		30 * preferencesModel.uiScale
		property int	extraWidth:		5 * padding + dropdownIcon.width


						isEmptyValue:			comboBox.addEmptyValue && comboBox.currentIndex === 0
						showEmptyValueStyle:	!comboBox.showEmptyValueAsNormal && isEmptyValue

						showIcon:				comboBox.showVariableTypeIcon && comboBox.currentColumnType && !control.isEmptyValue
						iconSource:				!visible ? "" : comboBox.currentColumnTypeIcon
						showBorder:				comboBox.showBorder
						useExternalBorder:		comboBox.useExternalBorder
						enabledOptions:			comboBox.enabledOptions

						addEmptyValue:			comboBox.addEmptyValue
						showEmptyValueAsNormal:	comboBox.showEmptyValueAsNormal
						addLineAfterEmptyValue:	comboBox.addLineAfterEmptyValue
						showVariableTypeIcon:	comboBox.showVariableTypeIcon

						placeholderText:		comboBox.placeholderText
						addScrollBar:			addScrollBar

		}

		delegate: ItemDelegate
		{
			height:								jaspTheme.comboBoxHeight
			width:								comboBoxBackground.width
			enabled:							comboBox.enabledOptions.length == 0 || comboBox.enabledOptions.length <= index || comboBox.enabledOptions[index]

			contentItem: Rectangle
			{
				id:								itemRectangle
				anchors.fill:					parent
				anchors.rightMargin:			scrollBar.visible ? scrollBar.width + 2 : 0
				color:							comboBox.currentIndex === index ? jaspTheme.itemSelectedColor : (control.highlightedIndex === index ? jaspTheme.itemHoverColor : jaspTheme.controlBackgroundColor)

				property bool isEmptyValue:		comboBox.addEmptyValue && index === 0
				property bool showEmptyValueStyle:	!comboBox.showEmptyValueAsNormal && isEmptyValue
				property bool showLine:			comboBox.addLineAfterEmptyValue && index === 0


				Image
				{
					id:							delegateIcon
					x:							1 * preferencesModel.uiScale
					height:						15 * preferencesModel.uiScale
					width:						15 * preferencesModel.uiScale
					source:						visible ? model.columnTypeIcon : ""
					visible:					comboBox.showVariableTypeIcon && !itemRectangle.isEmptyValue

					anchors.verticalCenter:		parent.verticalCenter
				}

				Text
				{
					x:							(delegateIcon.visible ? 20 : 4) * preferencesModel.uiScale
					text:						itemRectangle.isEmptyValue ? comboBox.placeholderText : (model && model.name ? model.name : "")
					font:						jaspTheme.font
					color:						itemRectangle.showEmptyValueStyle || !enabled ? jaspTheme.grayDarker : (comboBox.currentIndex === index ? jaspTheme.white : jaspTheme.black)
					anchors.verticalCenter:		parent.verticalCenter
					anchors.horizontalCenter:	itemRectangle.showEmptyValueStyle ? parent.horizontalCenter : undefined
				}

				Rectangle
				{
					anchors
					{
						left: parent.left
						right: parent.right
						bottom: parent.bottom
					}
					visible:	itemRectangle.showLine
					height:		1
					color:		jaspTheme.focusBorderColor
				}
			}
		}
    }
}
