#ifndef LABELFILTERGENERATOR_H
#define LABELFILTERGENERATOR_H

#include <QObject>

class DataSetQ;
class FilterQ;
///
/// This is used to generate R-filters based on what the user disables/enables in the label-editor (or variableswindow)
class LabelFilterGenerator : public QObject
{
	Q_OBJECT

public:
				LabelFilterGenerator(FilterQ * filter);
	std::string generateFilter();

public slots:
	void		regenerateGeneratedFilter();

private:
	FilterQ *	_filter = nullptr;
};

#endif // LABELFILTERGENERATOR_H
