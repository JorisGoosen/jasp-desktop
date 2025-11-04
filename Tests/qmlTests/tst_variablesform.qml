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
		id:				spyAssigned
		target:			vars
		signalName:		"countChanged"
	}

	SignalSpy
	{
			id:			spyError
			target:		vars
			signalName: "hasErrorChanged"
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

	function test_variableLists()
	{
		spyLoader.wait(400)
		compare(spyLoader.count, 1);
		compare(allVars.count, 1);

		allVars.setSelectedItem(0)
		allVars.moveSelectedItems(vars);

		spyAssigned.clear()
		spyAssigned.wait(400);
		compare(spyAssigned.count,	1);
		compare(allVars.count,		0);
		compare(vars.count,			1);

		vars.setSelectedItem(0)
		vars.moveSelectedItems(allVars);

		spyAssigned.clear()
		spyAssigned.wait(400);
		compare(spyAssigned.count,	1);
		compare(allVars.count,		1);
		compare(vars.count,			0);

		allVars.setSelectedItem(0)
		allVars.moveSelectedItems(vars);

		spyAssigned.clear()
		spyAssigned.wait(400);
		compare(spyAssigned.count,	1);
		compare(allVars.count,		0);
		compare(vars.count,			1);


		spyError.clear();
		vars.minNumericLevels = 10; //TestColumn has only 5
		spyError.wait(500);
		compare(vars.hasError,			true);


		spyError.clear();
		vars.minNumericLevels = 2;
		spyError.wait(500);
		compare(vars.hasError,			false);
	}
}
