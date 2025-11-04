import QtTest
import QtQuick
import QtQuick.Controls
import JASP.Controls


TestCase
{
	
	SignalSpy
	{
		id:				spyLoader
		target:			resultsView
		signalName:		"resultsLoaded"
	}

	VariablesForm
	{
		AvailableVariablesList{  name: "allVars"; 	id: allVars }
		AssignedVariablesList {  name: "vars";		id:	vars; }  //		title: qsTr("Variables"); allowedColumns: ["scale"]; minNumericLevels: 2 }
	}

	function test_rowcontrols()
	{
		compare(allVars.values().length, 5);
	}
}
