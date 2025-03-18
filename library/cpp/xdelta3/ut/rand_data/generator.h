#pragma once

#include <library/cpp/xdelta3/state/data_ptr.h>

NXdeltaAggregateColumn::TDataPtr RandData(size_t size);
NXdeltaAggregateColumn::TDataPtr RandDataModification(const ui8* in, size_t inSize, size_t size);
