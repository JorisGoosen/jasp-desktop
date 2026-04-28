import QtQuick
import QtQuick.Controls
import JASP.Controls		as JaspControls
import QtQml.Models

FocusScope
{
	id: __myRoot

	property alias model: dataTableView.model
	property alias visible: dataTableView.visible

	Rectangle
	{
		color:			jaspTheme.white
		anchors.fill:	parent
		z:				-1
		border.width:	1
		border.color:	jaspTheme.uiBorder

		JASPDataView
		{
			id:						dataTableView
			focus:					__myRoot.focus
			anchors.fill:			parent

			itemHorizontalPadding:	8 * jaspTheme.uiScale
			itemVerticalPadding:	8 * jaspTheme.uiScale

			cacheItems:				true

			editDelegate:			DataTableViewEdit {}
			itemDelegate:			DataTableViewItem {}
			rowNumberDelegate:		DataTableViewRowHeader {}
			columnHeaderDelegate:	DataTableViewColumnHeader {}
		}
	}
}
