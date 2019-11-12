import QtQuick			2.13
import QtQuick.Controls 2.13
import JASP.Widgets		1.0
import JASP.Controls	1.0

ScrollView
{
	id:						scrollPrefs
	focus:					true
	onActiveFocusChanged:	if(activeFocus) uiScaleSpinBox.forceActiveFocus();
	Keys.onLeftPressed:		resourceMenu.forceActiveFocus();

	Column
	{
		width:			scrollPrefs.width
		spacing:		jaspTheme.rowSpacing

		MenuHeader
		{
			id:				menuHeader
			headertext:		qsTr("Advanced Preferences")
			helpfile:		"preferences/prefsadvanced"
			anchorMe:		false
			width:			scrollPrefs.width - (2 * jaspTheme.generalMenuMargin)
			x:				jaspTheme.generalMenuMargin
		}


		PrefsGroupRect
		{
			title:				qsTr("Modules options")

			CheckBox
			{
				id:					rememberModulesSelected
				label:				qsTr("Remember Enabled Modules")
				checked:			preferencesModel.modulesRemember
				onCheckedChanged:	preferencesModel.modulesRemember = checked
				toolTip:			qsTr("Continue where you left of the next time JASP starts.\nEnabling this option makes JASP remember which Modules you've enabled.")
				KeyNavigation.tab:	developerMode
				KeyNavigation.down:	developerMode
			}

			CheckBox
			{
				id:					developerMode
				label:				qsTr("Developer mode (Beta version)")
				checked:			preferencesModel.developerMode
				onCheckedChanged:	preferencesModel.developerMode = checked
				toolTip:			qsTr("To use JASP Modules enable this option.")
				KeyNavigation.tab:	browseDeveloperFolderButton
				KeyNavigation.down:	browseDeveloperFolderButton
			}

			Item
			{
				id:					editDeveloperFolder
				visible:			preferencesModel.developerMode
				width:				parent.width
				height:				browseDeveloperFolderButton.height + overwriteDescriptionEtc.height

				RectangularButton
				{
					id:					browseDeveloperFolderButton
					text:				qsTr("Select developer folder")
					onClicked:			preferencesModel.browseDeveloperFolder()
					anchors.left:		parent.left
					anchors.leftMargin: jaspTheme.subOptionOffset
					toolTip:			qsTr("Browse to your JASP Module folder.")
					KeyNavigation.tab:	developerFolderText
					KeyNavigation.down:	developerFolderText
				}

				Rectangle
				{
					id:					developerFolderTextRect
					anchors
					{
						left:			browseDeveloperFolderButton.right
						right:			parent.right
						top:			parent.top
					}

					height:				browseDeveloperFolderButton.height
					color:				jaspTheme.white
					border.color:		jaspTheme.buttonBorderColor
					border.width:		1

					TextInput
					{
						id:					developerFolderText
						text:				preferencesModel.developerFolder
						clip:				true
						font:				jaspTheme.font
						onTextChanged:		preferencesModel.developerFolder = text
						color:				jaspTheme.textEnabled
						KeyNavigation.tab:	overwriteDescriptionEtc
						KeyNavigation.down:	overwriteDescriptionEtc
						selectByMouse:		true
						selectedTextColor:	jaspTheme.white
						selectionColor:		jaspTheme.itemSelectedColor


						anchors
						{
							left:			parent.left
							right:			parent.right
							verticalCenter:	parent.verticalCenter
							margins:		jaspTheme.generalAnchorMargin
						}

						Connections
						{
							target:					preferencesModel
							onCustomEditorChanged:	developerFolderText = preferencesModel.developerFolder
						}

					}
				}

				CheckBox
				{
					id:					overwriteDescriptionEtc
					label:				qsTr("Regenerate package metadata every time (DESCRIPTION & NAMESPACE)")
					checked:			preferencesModel.devModRegenDESC
					onCheckedChanged:	preferencesModel.devModRegenDESC = checked
					height:				implicitHeight * preferencesModel.uiScale
					toolTip:			qsTr("Disable this option if you are transforming your R-package to a JASP Module or simply want to keep manual changes to DESCRIPTION and NAMESPACE.")
					KeyNavigation.tab:	cranRepoUrl
					KeyNavigation.down:	cranRepoUrl

					anchors
					{
						left:			parent.left
						leftMargin:		jaspTheme.subOptionOffset
						top:			developerFolderTextRect.bottom
					}
				}
			}

			Item
			{
				id:		cranRepoUrlItem
				width:	parent.width
				height:	cranRepoUrlRect.height

				Label
				{
					id:		cranRepoUrlLabel
					text:	qsTr("Change the CRAN repository: ")

					anchors
					{
						left:			parent.left
						verticalCenter:	parent.verticalCenter
						margins:		jaspTheme.generalAnchorMargin
					}
				}

				Rectangle
				{
					id:					cranRepoUrlRect

					height:				browseDeveloperFolderButton.height

					color:				jaspTheme.white
					border.color:		jaspTheme.buttonBorderColor
					border.width:		1

					anchors
					{
						left:		cranRepoUrlLabel.right
						right:		parent.right
					}

					TextInput
					{
						id:					cranRepoUrl
						text:				preferencesModel.cranRepoURL
						clip:				true
						font:				jaspTheme.font
						onTextChanged:		preferencesModel.cranRepoURL = text
						color:				jaspTheme.textEnabled
						KeyNavigation.tab:	logToFile
						KeyNavigation.down:	logToFile
						selectByMouse:		true
						selectedTextColor:	jaspTheme.white
						selectionColor:		jaspTheme.itemSelectedColor


						anchors
						{
							left:			parent.left
							right:			parent.right
							verticalCenter:	parent.verticalCenter
							margins:		jaspTheme.generalAnchorMargin
						}
					}
				}
			}
		}

		PrefsGroupRect
		{
			id:		loggingGroup
			title:	qsTr("Logging options")

			CheckBox
			{
				id:					logToFile
				label:				qsTr("Log to file")
				checked:			preferencesModel.logToFile
				onCheckedChanged:	preferencesModel.logToFile = checked
				toolTip:			qsTr("To store debug-logs of JASP in a file, check this box.")
				KeyNavigation.tab:	maxLogFilesSpinBox
				KeyNavigation.down:	maxLogFilesSpinBox
			}

			Item
			{
				id:					loggingSubGroup
				x:					jaspTheme.subOptionOffset
				height:				maxLogFilesSpinBox.height
				width:				showLogs.x + showLogs.width
				enabled:			preferencesModel.logToFile


				SpinBox
				{
					id:					maxLogFilesSpinBox
					value:				preferencesModel.logFilesMax
					onValueChanged:		if(value !== "") preferencesModel.logFilesMax = value
					from:				5 //Less than 5 makes no sense as on release you get 1 for JASP-Desktop and 4 from the Engines
					to:					100
					defaultValue:		10
					stepSize:			1
					KeyNavigation.tab:	showLogs
					KeyNavigation.down:	showLogs
					text:				qsTr("Max logfiles to keep: ")

					anchors
					{
						leftMargin:	jaspTheme.generalAnchorMargin
						left:		parent.left
						top:		showLogs.top
						bottom:		showLogs.bottom
					}
				}

				RectangularButton
				{
					id:			showLogs
					text:		qsTr("Show logs")
					onClicked:	mainWindow.showLogFolder();
					anchors
					{
						margins:	jaspTheme.generalAnchorMargin
						left:		maxLogFilesSpinBox.right
					}
				}
			}
		}

	}
}
