#include "filterq.h"
#include "columnq.h"
#include "datasetq.h"
#include "jsonutilities.h"
#include "columnencoder.h"
#include "utilities/qutils.h"
#include "labelfiltergenerator.h"

FilterQ::FilterQ(DataSetQ * data)
:	Filter(data),
	_labelGen(new LabelFilterGenerator(this))
{
	connect(data, &DataSetQ::labelFilterChanged,		_labelGen,	&LabelFilterGenerator::regenerateGeneratedFilter	);
	connect(data, &DataSetQ::datasetChanged,			this,		&FilterQ::datasetChanged							);
	connect(this, &FilterQ::constructorRChanged,		_labelGen,	&LabelFilterGenerator::regenerateGeneratedFilter	);
	connect(this, &FilterQ::filteredRowCountChanged,	this,		&FilterQ::updateStatusBar							);
	connect(this, &FilterQ::filteredChanged,			data,		&DataSetQ::refreshAllAnalyses						);

	setRFilter(defaultRFilter());
}

FilterQ::FilterQ(DataSetQ * data, const std::string & name, bool createIfMissing)
:	Filter(data, name, createIfMissing)
{
	updateStatusBar();
}

DataSetQ * FilterQ::dataQ() const
{
	return static_cast<DataSetQ*>(data());
}


const char * FilterQ::defaultRFilter()
{
	static std::string defaultFilter;

	const std::string forceTranslatedStuffToAlwaysBeAComment =
		tr(
			"Above you see the code that JASP generates for both value filtering and the drag&drop filter."					"\n"
			"This default result is stored in 'generatedFilter' and can be replaced or combined with a custom filter."		"\n"
			"To combine you can append clauses using '&': 'generatedFilter & customFilter & perhapsAnotherFilter'"			"\n"
			"Click the (i) icon in the lower right corner for further help."												"\n").toStdString();

	defaultFilter = "# " + stringUtils::replaceBy(forceTranslatedStuffToAlwaysBeAComment, "\n", "\n# ") + "\n\ngeneratedFilter";

	return defaultFilter.c_str();
}

QString FilterQ::nameQ()			const { return tq(Filter::name());				}
QString FilterQ::rFilterQ()			const { return tq(Filter::rFilter());			}
QString FilterQ::constructorRQ()	const { return tq(Filter::constructorR());		}
QString FilterQ::filterErrorMsgQ()	const { return tq(Filter::errorMsg());			}
QString FilterQ::generatedFilterQ()	const { return tq(Filter::generatedFilter());	}
QString FilterQ::constructorJsonQ()	const { return tq(Filter::constructorJson());	}

bool FilterQ::columnUsed(const QString &name) const
{
	return columnsUsedInConstructor().count(fq(name)) > 0 || columnsUsedInRFilter().count(fq(name)) > 0;
}

bool FilterQ::hasFilter() const
{
	return rFilter() != defaultRFilter() || constructorJson() != DEFAULT_FILTER_JSON;
}

void FilterQ::setRFilter(			const QString &newRFilter			) { Filter::setRFilter(			fq(newRFilter));			}
void FilterQ::setConstructorR(		const QString &newConstructorR		) { Filter::setConstructorR(	fq(newConstructorR));		}
void FilterQ::setGeneratedFilter(	const QString &newGeneratedFilter	) { Filter::setGeneratedFilter(	fq(newGeneratedFilter));	}
void FilterQ::setConstructorJson(	const QString &newconstructorJson	) { Filter::setConstructorJson(	fq(newconstructorJson));	}
void FilterQ::setFilterErrorMsg(	const QString &newFilterErrorMsg	) { Filter::setErrorMsg(		fq(newFilterErrorMsg));		}

void FilterQ::setStatusBarText(const QString &newStatusBarText)
{
	if(newStatusBarText != _statusBarText)
	{
		_statusBarText = newStatusBarText;
		emit statusBarTextChanged();
	}
}

void FilterQ::updateStatusBar()
{
	int     TotalCount			= data()->rowCount(),
			TotalThroughFilter	= filteredRowCount();
	double	dblPercent			= 100.0 * ((double)TotalThroughFilter) / ((double)TotalCount);
	int		PercentageThrough	= (int)round(dblPercent);
	bool	Approximate			= std::abs(dblPercent - double(PercentageThrough)) > 0.000001;

	setStatusBarText(tr("Data has %1 rows, %2 (%3%4%) passed through filter").arg(TotalCount).arg(TotalThroughFilter).arg(Approximate ? "~" : "").arg(PercentageThrough));
}

void FilterQ::bindStdFunctionsToSignals()
{
	_nameChanged				= [this](){ emit nameChanged();				};
	_rFilterChanged				= [this](){ emit rFilterChanged();			};
	_filteredChanged			= [this](){ emit filteredChanged();			};
	_generatedFilterChanged		= [this](){ emit generatedFilterChanged();	};
	_filteredRowCountChanged	= [this](){ emit filteredRowCountChanged();	};
	_constructorJsonChanged		= [this](){ emit constructorJsonChanged();	};
	_constructorRChanged		= [this](){ emit constructorRChanged();		};
	_invalidatedChanged			= [this](){ emit invalidatedChanged();		};
	_errorMsgChanged			= [this](){ emit filterErrorMsgChanged();	};
}


void FilterQ::datasetChanged(		QStringList             changedColumns,
									QStringList             missingColumns,
									QMap<QString, QString>	changeNameColumns,
									bool                    rowCountChanged,
									bool                  /*hasNewColumns*/)
{
	bool invalidateMe = rowCountChanged;


	if(!invalidateMe)
		for(const QString & changed : changedColumns)
			if(columnUsed(changed))
			{
				invalidateMe = true;
				break;
			}

	auto iUseOneOfTheseColumns = [&](QStringList cols) -> bool
	{
		for(const QString & col : cols)
			if(columnUsed(col))
				return true;
		return false;
	};

	if(iUseOneOfTheseColumns(changeNameColumns.keys()))
	{
		strstrmap stdChangeNameCols(fq(changeNameColumns));

		invalidateMe = true;

		setRFilter(			tq(ColumnEncoder::replaceColumnNamesInRScript(rFilter(),							stdChangeNameCols)));
		setConstructorJson( tq(JsonUtilities::replaceColumnNamesInDragNDropFilterJSONStr(constructorJson(),		stdChangeNameCols)));
	}

	if(iUseOneOfTheseColumns(missingColumns))
	{
		Filter::setRFilter(ColumnEncoder::removeColumnNamesFromRScript(rFilter(), missingStd));
		Filter::setConstructorJson(JsonUtilities::removeColumnsFromDragNDropFilterJSONStr( constructorJson(), missingStd));

		invalidateMe = false; //Actually, if stuff is removed from the filter it won't work will it now?

		//Just reset the filter result to everything true whiol->hasLabelFille the user gets the change to fix their now broken filter
		reset();

		emit refreshAllAnalyses();

		updateStatusBar();

		//The following errormsg is overwritten immediately but that is because constructorJson changed triggers qml which triggers (some vents later) a send event. So yeah...
		//Ill leave it here though because it would be nice to show this friendlier msg then "null not found"
		setFilterErrorMsg("Some columns were removed from the data and your filter(s)!");
	}

	if(invalidateMe)
		runFilter();
}



