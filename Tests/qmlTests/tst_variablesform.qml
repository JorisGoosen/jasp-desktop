import QtTest
import QtQuick
import QtQuick.Controls
import JASP.Controls


TestCase
{

	SignalSpy
	{
		id:				spyLoader
		target:			jaspForm
		signalName:		"formCompletedSignal"
	}


	SignalSpy
	{
		id:				spyVarLoader
		target:			allVars
		signalName:		"initializedChanged"
	}



	Form
	{
		id:		jaspForm

		VariablesForm
		{
			id:			varsForm
			AvailableVariablesList{  name: "allVars"; 	id: allVars }
			AssignedVariablesList {  name: "vars";		id:	vars; }  //		title: qsTr("Variables"); allowedColumns: ["scale"]; minNumericLevels: 2 }
		}
	}

	function test_rowcontrols()
	{
		spyLoader.wait(1000)
		compare(spyLoader.count, 1);
		compare(allVars.count, 1);
		compare(allVars.num, 1);

		//Assign stuff in vars I guess?
	}
}
