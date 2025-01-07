#include "labelfiltergenerator.h"
#include "utilities/qutils.h"
#include "datasetq.h"
#include "filterq.h"
#include "columnq.h"
#include "timers.h"

LabelFilterGenerator::LabelFilterGenerator(FilterQ * filter)
	: QObject(filter), _filter(filter)
{
	connect(_filter->dataQ(),	&DataSetQ::labelFilterChanged,	this,	&LabelFilterGenerator::regenerateGeneratedFilter	);
	connect(_filter->dataQ(),	&DataSetQ::allFiltersReset,		this,	&LabelFilterGenerator::regenerateGeneratedFilter	);
}

std::string LabelFilterGenerator::generateFilter()
{
	JASPTIMER_SCOPE(LabelFilterGenerator::generateFilter);

	int neededFilters = 0;

	for(Column * c : _filter->data()->columns())
		if(c->hasLabelFilter())
			neededFilters++;

	std::stringstream newGeneratedFilter;

	std::string filterRScript = _filter->constructorR();

	newGeneratedFilter << "generatedFilter <- ";

	if(neededFilters == 0)
	{
		if(filterRScript == "")	return DEFAULT_FILTER_GEN;
		else					newGeneratedFilter << "("<< filterRScript <<")";
	}
	else
	{
		bool	moreThanOne = neededFilters > 1,
				first		= true;

		if(moreThanOne)
			newGeneratedFilter << "(";
		
		for(ColumnQ * c : _filter->dataQ()->columnsQ())
			if(c->hasLabelFilter())
			{
				newGeneratedFilter << (first ? "" : " & ") << c->generateLabelFilter();
				first = false;
			}

		if(moreThanOne)
			newGeneratedFilter << ")";

		if(filterRScript != "")
			newGeneratedFilter << " & \n("<<filterRScript<<")";
	}

	return newGeneratedFilter.str();
}

void LabelFilterGenerator::regenerateGeneratedFilter()
{
	JASPTIMER_SCOPE(LabelFilterGenerator::regenerateGeneratedFilter);

	_filter->setGeneratedFilter(tq(generateFilter()));
}


