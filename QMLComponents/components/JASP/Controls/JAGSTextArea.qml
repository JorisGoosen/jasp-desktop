import QtQuick			2.0
import JASP.Controls	1.0
import JASP				1.0

TextArea
{
	textType: JASP.TextTypeJAGSmodel
	showLineNumber: true
	
	Accessible.description:		info === undefined || info == "" ? toolTip !== undefined && toolTip != "" ? toolTip :  qsTr("A JAGS text area %1").arg(title) : info
	
	RSyntaxHighlighterQuick
	{
		textDocument:		parent.textDocument
	}
}
