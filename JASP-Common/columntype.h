#ifndef COLUMNTYPE_H
#define COLUMNTYPE_H
#include "enumutilities.h"

DECLARE_ENUM(ColumnType, ColumnTypeUnknown = 0, ColumnTypeNominal = 1, ColumnTypeNominalText = 2, ColumnTypeOrdinal = 4, ColumnTypeScale = 8);

#endif // COLUMNTYPE_H
