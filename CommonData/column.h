#ifndef COLUMN_H
#define COLUMN_H

#include "datasetbasenode.h"
#include "label.h"
#include "columntype.h"
#include "utils.h"
#include <list>
#include "emptyvalues.h"

class DataSet;
class Analysis;

/// A column of data
/// 
/// It can have 3 columnTypes, scalar, ordinal or nominal (nominalText is a relict of the past).
/// 
/// The data itself is stored in a hybrid fashion, where any values are stored in double (and thus int)-format in _dbls.
/// Anything with a label on it will have a Label-object in Column and the "intsId" of this label is entered in the corresponding _ints row.
/// If the originalValue of that label is convertible to double/int it will *also* be stored in _dbls. (Which is different from how <0.19 JASPs did it)
/// 
/// If no label exists _ints simply contains Label::DOUBLE_LABEL_VALUE (-1) and it tells JASP that _dbl should be used.
/// We do want users to be able to edit them, or to set "filter allows" or something on it.
/// To this end labelsTempCount() can be called to get the total of "labels" a column has.
/// The shown labels are stored in a temporary internal representation (stringvec).
/// 
/// What this means is that a column could have "labels" visible in the label-editor, but _labels.size() == 0!
/// This is great because changing a column with 1million unique doubles doesnt need 1 million new labels, but no operations at all.
/// Pretty nice
/// 
/// However, this does mean that things like reversing labels or disabling filterAllows on a labels requires creating all those labels.
/// This is done with Column::replaceDoubleWithLabel(*) and co.
/// 
/// The logic of getting "labels" or "values" from a column is now in Column::dataAsRLevels Column::dataAsRDoubles instead of in rbridge.
/// Although the conversion to whatever R expects is still done there (rbridge_readDataSet). But perhaps it could be moved further to the jaspRcpp side of things.
/// 
/// It has a variety of utility functions to modify, reorder or init values in the column.
/// As well as UI support functions for modifying the labels and such.
/// 
/// It also handles storing the information of computed columns (those used to be split off)
class Column : public DataSetBaseNode
{
public:
									Column(DataSet * data, int id = -1);
									~Column();
									
				DatabaseInterface & db();
		const	DatabaseInterface & db() const;

			void					dbCreate(	int index);
			void					dbLoad(		int id=-1, bool getValues = true);	///< Loads *and* reloads from DB!
			void					dbLoadIndex(int index, bool getValues = true);
			void					dbUpdateComputedColumnStuff();
			void					dbUpdateValues(bool labelsTempCanBeMaintained = true);
			void					dbDelete(bool cleanUpRest = true);
																														
			
			void					setName(			const std::string & name			);
			void					setTitle(			const std::string & title			);
			bool					setRCode(			const std::string & rCode			);
			bool					setError(			const std::string & error			);
			void					setType(			columnType			colType			);
			columnTypeChangeResult	changeType(			columnType			colType			);
			void					setCodeType(		computedColumnType	codeType		);
			void					setDescription(		const std::string & description		);
			bool					setConstructorJson(	const Json::Value & constructorJson	);
			bool					setConstructorJson(	const std::string & constructorJson	);
			void					setAnalysisId(		int					analysisId		);
			void					setInvalidated(		bool				invalidated		);
			void					setCompColStuff(bool   invalidated, computedColumnType   codeType, const	std::string & rCode, const	std::string & error, const	Json::Value & constructorJson);
			void					setDefaultValues(enum columnType columnType = columnType::unknown);

			bool					setAsNominalOrOrdinal(	const intvec	& values,									bool	is_ordinal = false);
			bool					setAsNominalOrOrdinal(	const intvec	& values, intstrmap uniqueValues,			bool	is_ordinal = false);


			bool					overwriteDataWithScale(				doublevec	scalarData);
			bool					overwriteDataWithOrdinalOrNominal(	bool is_ordinal, intvec		ordinalData, intstrmap levels = {});
			bool					overwriteDataWithNominal(			stringvec	nominalData);
			
			bool					allLabelsPassFilter()	const;
			bool					hasFilter()				const;
			void					resetFilter();
			void					incRevision(bool labelsTempCanBeMaintained = true);
			bool					checkForUpdates();

			bool					isColumnDifferentFromStringValues(const stringvec & strVals) const;

			columnType				type()					const	{ return _type;				}
			int						id()					const	{ return _id;				}
			int						analysisId()			const	{ return _analysisId;		}
			bool					isComputed()			const	{ return _codeType != computedColumnType::notComputed && _codeType != computedColumnType::analysisNotComputed;	}
			bool					invalidated()			const	{ return _invalidated;		}
			computedColumnType		codeType()				const	{ return _codeType;			}
			const std::string	&	name()					const	{ return _name;				}
			const std::string	&	title()					const	{ return _title.empty() ? _name : _title;	}
			const std::string	&	description()			const	{ return _description;		}
			const std::string	&	error()					const	{ return _error;			}
			const std::string	&	rCode()					const	{ return _rCode;			}
				  std::string		rCodeStripped()			const	{ return stringUtils::stripRComments(_rCode);	}
				  std::string		constructorJsonStr()	const	{ return _constructorJson.toStyledString();	}
			const Json::Value	&	constructorJson()		const	{ return _constructorJson;	}
			size_t					rowCount()				const	{ return _dbls.size(); }
			const intvec		&	ints()					const	{ return _ints; }
			const doublevec		&	dbls()					const	{ return _dbls; }
			
			void					upgradeSetDoubleLabelsInInts();			///< Used by upgrade 0.18.* -> 0.19
			void					upgradeExtractDoublesIntsFromLabels();	///< Used by upgrade 0.18.* -> 0.19

			void					labelsClear();
			int						labelsAdd(			int display);
			int						labelsAdd(			const std::string & display);
			int						labelsAdd(			const std::string & display, const std::string & description, const Json::Value & originalValue);
			int						labelsAdd(			int value, const std::string & display, bool filterAllows, const std::string & description, const Json::Value & originalValue, int order=-1, int id=-1);
			void					labelsRemoveValues(	intset valuesToRemove);
			strintmap				labelsResetValues(	int & maxValue);
			void					labelsRemoveBeyond( size_t indexToStartRemoving);
			
			int						labelsTempCount(); ///< Generates the labelsTemp also!
			const stringvec		&	labelsTemp();
			std::string				labelsTempDisplay(	size_t tempLabelIndex);
			std::string				labelsTempValue(	size_t tempLabelIndex, bool fancyEmptyValue = false);
			Label				*	labelDoubleDummy()		{ return _doubleDummy; }

			bool					labelsSyncInts(		const intset	& dataValues);
			bool					labelsSyncIntsMap(	const intstrmap	& dataValues);
			strintmap				labelsSyncStrings(	const stringvec	& new_values, const strstrmap &new_labels, bool * changedSomething = nullptr);

			std::set<size_t>		labelsMoveRows(std::vector<qsizetype> rows, bool up);
			void					labelsReverse();

			std::string				operator[](	size_t row); ///< Display value/label for row
			std::string				getValue(	size_t row,	bool fancyEmptyValue = false)	const; ///< Returns the ("original") value. Basically whatever the user would like to see as value. Stored internally as json
			std::string				getDisplay(	size_t row,	bool fancyEmptyValue = true)	const;
			std::string				getLabel(	size_t row,	bool fancyEmptyValue = false)	const;
			stringvec				valuesAsStrings()										const;
			stringvec				labelsAsStrings()										const;
			stringvec				displaysAsStrings()										const;
			stringvec				dataAsRLevels(intvec & values, const boolvec & filter, bool useLabels = true)		const; ///< values is output! If filter is of different length than the data an error is thrown, if length is zero it is ignored. useLabels indicates whether the levels will be based on the label or on the value as specified in the label editor.
			doublevec				dataAsRDoubles(const boolvec & filter)						const; ///< If filter is of different length than the data an error is thrown, if length is zero it is ignored
			std::map<double,Label*>	replaceDoubleWithLabel(doublevec dbls);
			Label				* 	replaceDoubleWithLabel(double dbl);
                        Label				* 	replaceDoublesTillLabelsRowWithLabels(size_t row);
			void					labelValueChanged(Label * label, double aDouble); ///< Pass NaN for non-convertible values
			void					labelValueChanged(Label * label, int	anInteger) { labelValueChanged(label, double(anInteger)); }
			void					labelDisplayChanged(Label * label);
			
			bool					setStringValueToRowIfItFits(size_t row, const std::string & value);
			bool					setValue(					size_t row, int					value, bool writeToDB = true);
			bool					setValue(					size_t row, double				value, bool writeToDB = true);
			bool					setValue(					size_t row, int					valueInt, double valueDbl, bool writeToDB = true);
			columnType				setValues(	const stringvec &	values, const stringvec &	labels, int thresholdScale, bool * changedSomething = nullptr); ///< Returns what would be the most sensible columntype
			columnType				setValues(	const stringvec &	values,								int thresholdScale, bool * changedSomething = nullptr); ///< Returns what would be the most sensible columntype
			bool					setDescriptions(			strstrmap labelToDescriptionMap); ///<Returns any changes
			void					rowInsertEmptyVal(size_t row);
			void					rowDelete(size_t row);
			void					setRowCount(size_t row);

			Labels				&	labels()															{ return _labels; }
			const Labels		&	labels()													const	{ return _labels; }
			Label				*	labelByIntsId(			int						intsId)		const; ///< Might be nullptr for missing label
			Label				*	labelByDisplay(			const std::string	&	display)	const; ///< Might be nullptr for missing label
			Label				*	labelByValue(			const std::string	&	value)		const; ///< Might be nullptr for missing label
			Label				*	labelByRow(				int						row)		const; ///< Might be nullptr for missing label
			int						labelIndex(				const Label			*	label)		const;

			bool					isValueEqual(size_t row, double value)				 const;
			bool					isValueEqual(size_t row, int value)					 const;
			bool					isValueEqual(size_t row, const std::string &value)	 const;

			intset					getUniqueLabelValues() const;
			
			void					beginBatchedLabelsDB();
			void					endBatchedLabelsDB(bool wasWritingBatch = true);
			bool					batchedLabel()	{ return _batchedLabel; }
			
			DataSet				*	data() const { return _data; }

			void					loadComputedColumnJsonBackwardsCompatibly(const Json::Value & fromJaspFile);
			void					invalidate()																			{ setInvalidated(true);		}
			void					validate()																				{ setInvalidated(false);	}
			void					invalidateDependents();
			bool					hasError()																	const		{ return !error().empty();	}
			void					findDependencies();
			void					setDependsOn(const stringset & columns);
			bool					dependsOn(const std::string & columnName, bool refresh = true);
			bool					iShouldBeSentAgain();
			bool					isComputedByAnalysis(size_t analysisID);

			void					checkForLoopInDependencies(std::string code);
			const	stringset	 &	dependsOnColumns(bool refresh = true);
			Json::Value				serialize()																const;
			void					deserialize(const Json::Value& info);
			std::string				getUniqueName(const std::string& name)									const;
			std::string				doubleToDisplayString(	double dbl, bool fancyEmptyValue = true)		const; ///< fancyEmptyValue is the user-settable empty value label, for saving to csv this might be less practical though, so turn it off
			bool					hasCustomEmptyValues()													const;
	const   EmptyValues    		*	emptyValues()															const { return _emptyValues; }
			void					setHasCustomEmptyValues(		bool hasCustom);
			bool					setCustomEmptyValues(			const stringset		& customEmptyValues); ///<returns whether there was a change
			
			bool					isEmptyValue(					const std::string	& val)															const;
			bool					isEmptyValue(					const double		  val)															const;
			
			
			qsizetype				getMaximumWidthInCharacters(bool shortenAndFancyEmptyValue, bool valuesPlease);
			columnType				resetValues(int thresholdScale);
			stringset				mergeOldMissingDataMap(const Json::Value & missingData); ///< <0.19 JASP collected the removed empty values values in a map in a json object... We need to be able to read at least 0.18.3 so here this function that absorbs such a map and adds any required labels. It does not add the empty values itself though!

protected:
			void					_checkForDependencyLoop(stringset foundNames, std::list<std::string> loopList);
			void					_dbUpdateLabelOrder(bool noIncRevisionWhenBatchedPlease = false);		///< Sets the order of the _labels to label.order and in DB
			void					_sortLabelsByOrder();		///< Sorts the labels by label.order
			std::string				_getLabelDisplayStringByValue(int key) const;
			columnTypeChangeResult	_changeColumnToNominalOrOrdinal(enum columnType newColumnType);
			columnTypeChangeResult	_changeColumnToScale();
			void					_convertVectorIntToDouble(intvec & intValues, doublevec & doubleValues);
			void					_resetLabelValueMap();
			
			void					labelsTempReset();

private:
			DataSet		*			_data				= nullptr;
			EmptyValues	*			_emptyValues		= nullptr;			
			Labels					_labels;
			Label		*			_doubleDummy;		///< Only used to work around node problems in DataSetPackage. Should probably be replaced with something less hacky later on. (when rewriting DataSetPackage models)
			columnType				_type				= columnType::unknown;
			int						_id					= -1,
									_analysisId			= -1,		// Actually initialized in DatabaseInterface::columnInsert
									_labelsTempRevision	= -1;	///< When were the "temporary labels" created?
			qsizetype				_labelsTempMaxWidth = 0;
			stringvec				_labelsTemp;				///< Contains displaystring for labels. Used to allow people to edit "double" labels. Initialized when necessary
			strintmap				_labelsTempToIndex;
			bool					_invalidated		= false,
									_batchedLabel		= false;
			computedColumnType		_codeType			= computedColumnType::notComputed;
			std::string				_name,
									_title,
									_description,
									_error,
									_rCode;
			Json::Value				_constructorJson	= Json::objectValue;
			doublevec				_dbls;
			intvec					_ints;
			stringset				_dependsOnColumns;
			std::map<int, Label*>	_labelByIntsIdMap;
			
			
};

typedef std::vector<Column*> Columns;

#endif // COLUMN_H
