#ifndef COLUMNTYPE_H
#define COLUMNTYPE_H
#include "enumutilities.h"

/* replace by:
	{ columnType::ColumnTypeNominalText	, "nominalText" },
	{ columnType::ColumnTypeNominal		, "nominal"},
	{ columnType::ColumnTypeOrdinal		, "ordinal"},
	{ columnType::ColumnTypeScale		, "scale"}
	*/

DECLARE_ENUM(columnType, ColumnTypeUnknown = 0, ColumnTypeNominal = 1, ColumnTypeNominalText = 2, ColumnTypeOrdinal = 4, ColumnTypeScale = 8);

#endif // COLUMNTYPE_H
