import QtQuick
import QtQuick.Controls
import JASP.Controls	as JaspControls
import "FilterConstructor"
import JASP
import QtQuick.Layouts

Item
{
	id:					root
	implicitHeight:		theButton.implicitHeight
	implicitWidth:		theButton.width
	height:				implicitHeight
	width:				implicitWidth
	
	property alias	text:			theButton.text
	property alias	iconSource:		theButton.iconSource
	property string	name:			""
	property string	description:	""
	property bool	buttonActive:	false
	property bool	showTextField:	false
	property alias	theButton:		theButton
	property bool   isComputed:		false;

	signal clicked();
	signal doubleClicked();
	
	Loader
	{
		id:					textFieldLoader
		sourceComponent:	showTextField ? textField : undefined
		anchors.centerIn:	theButton
		z:					10
		
	}
	
	JaspControls.RoundedButton
	{
		id:			theButton
		height:		implicitHeight + offset
		width:		showTextField ? Math.max(implicitWidth, textFieldLoader.width + jaspTheme.generalAnchorMargin) : implicitWidth
		
		property real offset: 4 * jaspTheme.uiScale
		
		y:		-1 * offset + 1
		
		onClicked:			parent.clicked()
		onDoubleClicked:	parent.doubleClicked()
		color:				buttonActive ? jaspTheme.white : jaspTheme.buttonColor
		enabled:			!buttonActive && !showTextField
		textColor:			!showTextField ? jaspTheme.textEnabled : theButton.color;
		
		Rectangle
		{
			id:				disabledTabButtonLineBottom
			color:			parent.border.color
			height:			1
			visible:		!buttonActive
			anchors
			{
				left:			parent.left
				right:			parent.right
				bottom:			parent.bottom
				bottomMargin:	parent.implicitHeight
			}
		}
		
		toolTip:		description
	}
	
	Component
	{
		id:	textField
		
		RowLayout
		{
			CheckBox
			{
				id:					computedToggle
				checked:			root.isComputed
				onCheckedChanged:	dataSetPackage.workspace.setDataSetComputed(root.name, checked)

				ToolTip.text:		qsTr("Computed dataset")
				ToolTip.visible:	hovered
			}
			
			TextInput
			{
				id:		textInput
				font:	jaspTheme.font
				color:	jaspTheme.textEnabled
				text:	theButton.text
				
				onEditingFinished:	dataSetPackage.workspace.shownDataSet.title = text
			}
			
			JaspControls.MenuButton
			{
				id:					deleteButton
				height:				textInput.height
				width:				height
				radius:				height
				iconSource:			jaspTheme.iconPath + "close-button.png"
				onClicked:			dataSetPackage.workspace.deleteShownDataSet()
				toolTip:			qsTr("Delete dataset")
			}
			
			
		}
	}
}
