//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

import QtQuick			2.12
import QtWebEngine		1.7
import QtWebChannel		1.0
import JASP.Widgets		1.0
import JASP.Controls	1.0
import QtQuick.Controls 6.0

/// The main principle here will be that we want to have 3 views (or 2 when no data)
/// This will be shown in a splitview as: [ (DataPanel,) AnalysisForms, ResultsWebEngine ]
/// Previously there was some messing about outside of the screen to the left and right
/// but that only leads to problems and makes this qml more complicated than necessary...
/// So, without doing that again the aim should be:
/// Make sure the separators are easy to use, that the arrowbuttons in the handles turn leftward only when at maximum width
/// We also want to be able to softly close the analysisforms by dragging it left,
/// to expose more of the results while keeping some of the form visible. The analyses forms should then slight out to the "left"
/// The datapanel on the other hand will just be anchored to the left inside its window and dragging or using the arrows just exposes it
/// The results window will also be anchored to the left and should have extra width inside its window.
/// That extra width should be the same width as its possible max when analyses and data are collapsed.
/// Otherwise the webengine needs to change layout too much.
///
/// Other behaviour should include the way that datapanel (or results for that matter) stay maximized if the jasp windowsize changes.
/// Also, when new analyses are added the results + analysisform should both be in screen if possible (and otherwise at least the analyses)
Item
{
	id: splitViewContainer

	/*
	onWidthChanged:
	{
		if(!mainWindow.analysesAvailable)												data.maximizeData();
		else if(data.wasMaximized)														return; //wasMaximized binds!
		else if(splitViewContainer.width <= data.width + jaspTheme.splitHandleWidth)	data.maximizeData();
	}*/

	//onVisibleChanged:	if(visible && !mainWindow.dataPanelVisible)	data.minimizeData();

	SplitView
	{
		id:				panelSplit
		orientation:	Qt.Horizontal
		anchors.fill:	parent

		DataPanel
		{
			id:						data
			z:						1

			property real maxWidth:			! mainWindow.dataAvailable ? 0 : splitViewContainer.width - (mainWindow.analysesAvailable ?  2 * jaspTheme.splitHandleWidth : 0)
			SplitView.maximumWidth:			maxWidth
			SplitView.preferredWidth:		maxWidth

			/*property real baseMaxWidth:					fakeEmptyDataForSumStatsEtc ? 0 : splitViewContainer.width - (mainWindow.analysesAvailable ? jaspTheme.splitHandleWidth : 0)
			property bool fakeEmptyDataForSumStatsEtc:	!mainWindow.dataAvailable && mainWindow.analysesAvailable
			property bool wasMaximized:					false

			signal makeSureHandleVisible()

			function onMakeSureHandleVisible()
			{
				if(data.width < leftHandSpace)
					data.minimizeData();
				else if(data.width - leftHandSpace > splitViewContainer.width - jaspTheme.splitHandleWidth)
					data.maximizeData();
			}

			onWidthChanged:
			{

				var iAmBig = width > leftHandSpace;
				if(iAmBig != mainWindow.dataPanelVisible)
					mainWindow.dataPanelVisible = iAmBig

				if(fakeEmptyDataForSumStatsEtc)
				{
					mainWindow.dataPanelVisible = false;
					width = leftHandSpace;
				}

				if(data.width != data.maxWidth)
					data.wasMaximized = false;

				makeSureHandleVisible();
			}

			function maximizeData()	{ SplitView.preferredWidth = Qt.binding(function() { return data.maxWidth; });	data.wasMaximized = true;  }
			function minimizeData()	{ SplitView.preferredWidth = Qt.binding(function() { return leftHandSpace; });	data.wasMaximized = false; }

			Connections
			{
				target:		mainWindow
				function onDataPanelVisibleChanged(visible)
				{
					if (visible && data.width		<=	data.leftHandSpace)	data.maximizeData();
					else if(!visible)										data.minimizeData();
				}
			}

			Connections
			{
				target:	analysesModel
				function onVisibleChanged(visible)
				{
					if(!visible)
						data.makeSureHandleVisible();
				}
			}*/
		}


		handle:
			AnalysesSplitHandle
			{
				id:					splitHandle
				z:					1000
				splitView:			panelSplit
				/*containmentMask:	Item
				{
					width:			splitHandle.implicitWidth
					height:			splitHandle.height
				}*/
			}



		Rectangle
		{
			id:								giveResultsSomeSpace
			property int maxResultsWidth:	splitViewContainer.width - (2 * jaspTheme.splitHandleWidth)
			SplitView.maximumWidth:			maxResultsWidth
			SplitView.preferredWidth:		jaspTheme.resultWidth
			SplitView.fillWidth:			true
			z:								3
			visible:						mainWindow.analysesAvailable
			//onVisibleChanged:				if(visible) width = jaspTheme.resultWidth; else data.maximizeData()
			color:							analysesModel.currentAnalysisIndex !== -1 ? jaspTheme.uiBackground : jaspTheme.white

			/*Connections
			{
				target:				analysesModel
				function			onAnalysisAdded()
				{
					//make sure we get to see the results!

					var inputOutputWidth	= splitViewContainer.width - (data.width + jaspTheme.splitHandleWidth)
					var remainingDataWidth	= Math.max(0, data.width - (jaspTheme.splitHandleWidth + jaspTheme.resultWidth));

					if(inputOutputWidth < 100 * preferencesModel.uiScale)
						 mainWindow.dataPanelVisible = false;
					else if(inputOutputWidth < jaspTheme.resultWidth)
					{
						if(remainingDataWidth === 0)	mainWindow.dataPanelVisible = false;
						else							data.width = remainingDataWidth
					}
				}
			}*/

			WebEngineView
			{
				id:						resultsView

				anchors
				{
					top:				parent.top
					bottom:				parent.bottom
				}

				x:						1 + (Math.floor(parent.x) - parent.x)
				width:					giveResultsSomeSpace.maxResultsWidth

				url:					resultsJsInterface.resultsPageUrl
				onContextMenuRequested: (request)=>{ request.accepted = true; }
				backgroundColor:		jaspTheme.uiBackground

				Keys.onPressed: (event) =>
				{
					switch(event)
					{
					case Qt.Key_PageDown:	resultsView.runJavaScript("windows.pageDown();");	break;
					case Qt.Key_PageUp:		resultsView.runJavaScript("windows.pageUp();");		break;
					}
				}

				onNavigationRequested: (request)=>
				{
					if(request.navigationType === WebEngineNavigationRequest.ReloadNavigation || request.url == resultsJsInterface.resultsPageUrl)
						request.accept()
					else
					{
						if(request.navigationType === WebEngineNavigationRequest.LinkClickedNavigation)
							Qt.openUrlExternally(request.url);
						request.reject();
					}
				}

				onLoadingChanged: (loadRequest)=>
				{
					resultsJsInterface.resultsLoaded = loadRequest.status === WebEngineView.LoadSucceededStatus;
					setTranslatedResultsString();
				}



				Connections
				{
					target:		resultsJsInterface
					function onRunJavaScriptSignal(js)			{ resultsView.runJavaScript(js); }
					function onScrollAtAllChanged(scrollAtAll)	{ resultsView.runJavaScript("window.setScrollAtAll("+(scrollAtAll ? "true" : "false")+")"); }

					function onExportToPDF(pdfPath)
					{
						if(preferencesModel.currentThemeName !== "lightTheme")
							resultsJsInterface.setThemeCss("lightTheme");

						resultsJsInterface.unselect(); //Otherwise we get the selected analysis highlighted in the pdf...
						resultsView.printToPdf(pdfPath);
					}
				}
				onPdfPrintingFinished: (filePath)=>
				{
					if(preferencesModel.currentThemeName !== "lightTheme")
						resultsJsInterface.setThemeCss(preferencesModel.currentThemeName);

					resultsJsInterface.pdfPrintingFinished(filePath);
				}

				webChannel.registeredObjects:	[ resultsJsInterfaceInterface ]

				property string resultsString:	qsTr("Results")
				onResultsStringChanged:			setTranslatedResultsString();

				function setTranslatedResultsString()
				{
					if(resultsJsInterface.resultsLoaded)
						runJavaScript("window.setAnalysesTitle(\"" + resultsString + "\");");
				}

				QtObject
				{
					id:				resultsJsInterfaceInterface
					WebChannel.id:	"jasp"

					// Yeah I know this "resultsJsInterfaceInterface" looks a bit stupid but this honestly seems like the best way to make the current resultsJsInterface functions available to javascript without rewriting (more of) the structure of Desktop right now.
					// It would be much better to have resultsJsInterface be passed directly though..
					// It also gives you an overview of the functions used in results html

					function openFileTab()								{ resultsJsInterface.openFileTab()                              }
					function saveTextToFile(fileName, html)				{ resultsJsInterface.saveTextToFile(fileName, html)             }
					function analysisUnselected()						{ resultsJsInterface.analysisUnselected()                       }
					function analysisSelected(id)						{ resultsJsInterface.analysisSelected(id)                       }
					function analysisChangedDownstream(id, model)		{ resultsJsInterface.analysisChangedDownstream(id, model)       }
					function analysisTitleChangedInResults(id, title)	{ resultsJsInterface.analysisTitleChangedInResults(id, title)	}
					function analysisSaveImage(id, options)				{ resultsJsInterface.analysisSaveImage(id, options)				}
					function analysisEditImage(id, options)				{ resultsJsInterface.analysisEditImage(id, options)				}
					function removeAnalysisRequest(id)					{ resultsJsInterface.removeAnalysisRequest(id)					}
					function pushToClipboard(mime, raw, coded)			{ resultsJsInterface.pushToClipboard(mime, raw, coded)			}
					function pushImageToClipboard(raw, coded)			{ resultsJsInterface.pushImageToClipboard(raw, coded)			}
					function saveTempImage(index, path, base64)			{ resultsJsInterface.saveTempImage(index, path, base64)			}
					function getImageInBase64(index, path)				{ resultsJsInterface.getImageInBase64(index, path)				}
					function resultsDocumentChanged()					{ resultsJsInterface.resultsDocumentChanged()					}
					function displayMessageFromResults(msg)				{ resultsJsInterface.displayMessageFromResults(msg)				}
					function setAllUserDataFromJavascript(json)			{ resultsJsInterface.setAllUserDataFromJavascript(json)			}
					function setResultsMetaFromJavascript(json)			{ resultsJsInterface.setResultsMetaFromJavascript(json)			}
					function duplicateAnalysis(id)						{ resultsJsInterface.duplicateAnalysis(id)						}
					function showDependenciesInAnalysis(id, optName)	{ resultsJsInterface.showDependenciesInAnalysis(id, optName)	}

					function showAnalysesMenu(options)
					{
						// FIXME: This is a mess
						// TODO:  1. remove redundant computations
						//        2. move everything to one place :P

						var optionsJSON  = JSON.parse(options);
						var functionCall = function (index)
						{
							var name		= customMenu.props['model'].getName(index);
							var jsfunction	= customMenu.props['model'].getJSFunction(index);
							
							customMenu.hide()

							if (name === 'hasExportResults')				{ fileMenuModel.exportResultsInteractive();		return; }
							if (name === 'hasRefreshAllAnalyses')			{ resultsJsInterface.refreshAllAnalyses();		return;	}
							if (name === 'hasRemoveAllAnalyses')			{ resultsJsInterface.removeAllAnalyses();		return; }
							if (name === 'hasCopy' || name === 'hasCite')	  resultsJsInterface.purgeClipboard();

							resultsJsInterface.runJavaScript(jsfunction);

							if (name === 'hasEditTitle' || name === 'hasNotes')
								resultsJsInterface.packageModified();

						}

						var selectedOptions = []
						for (var key in optionsJSON)
							if (optionsJSON.hasOwnProperty(key) && optionsJSON[key] === true)
								selectedOptions.push(key)

						resultMenuModel.setOptions(options, selectedOptions);

						var props = {
							"model"			: resultMenuModel,
							"functionCall"	: functionCall
						};

						customMenu.toggle(resultsView, props, (optionsJSON['rXright'] + 10) * preferencesModel.uiScale, optionsJSON['rY'] * preferencesModel.uiScale);

						customMenu.scrollOri		= resultsView.scrollPosition;
						customMenu.menuScroll.x		= Qt.binding(function() { return -1 * (resultsView.scrollPosition.x - customMenu.scrollOri.x) / resultsView.zoomFactor; });
						customMenu.menuScroll.y		= Qt.binding(function() { return -1 * (resultsView.scrollPosition.y - customMenu.scrollOri.y) / resultsView.zoomFactor; });
						customMenu.menuMinIsMin		= true
					}
				}
			}
		}
	}
}
