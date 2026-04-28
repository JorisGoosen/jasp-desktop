import QtQuick
import QtQuick.Controls

FocusScope
{
	id: __myRoot

	Rectangle
	{
		color:			"purple" jaspTheme.white
		anchors.fill:	parent
		z:				-1
		border.width:	1
		border.color:	jaspTheme.uiBorder

		TableView
		{
			id:						dataTableView
			focus:					__myRoot.focus
			anchors.fill:			parent
			
			model:					csvPreviewModel

			//itemHorizontalPadding:	8 * jaspTheme.uiScale
			//itemVerticalPadding:	8 * jaspTheme.uiScale

			//cacheItems:				true

			//editDelegate:			DataTableViewEdit {}
			//itemDelegate:			DataTableViewItem {}
			//rowNumberDelegate:		DataTableViewRowHeader {}
			//columnHeaderDelegate:	DataTableViewColumnHeader {}
		}
	}
}
