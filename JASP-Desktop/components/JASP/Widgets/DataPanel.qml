import QtQuick			2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts	1.0
import JASP.Theme		1.0

Rectangle
{
	id:				rootDataset
	color:			Theme.uiBackground

	SplitView
    {
		id:				splitViewData
		anchors.fill:	parent
		orientation:	Qt.Vertical
		handle:			Rectangle { color: Theme.uiBorder; }

		FilterWindow
		{
			id:							filterWindow
			objectName:					"filterWindow"
			SplitView.maximumHeight:	rootDataset.height * 0.8
		}

		ComputeColumnWindow
		{
			id:							computeColumnWindow
			objectName:					"computeColumnWindow"
			SplitView.maximumHeight:	rootDataset.height * 0.8
		}

        VariablesWindow
        {
			id:							variablesWindow
			SplitView.minimumHeight:	calculatedMinimumHeight
        }

		DataTableView
		{
			objectName:				"dataSetTableView"
			SplitView.fillHeight:	true
			onDoubleClicked:		mainWindow.startDataEditorHandler()
        }
	}
}
