#pragma once
#include <util/system/types.h>

class IFactorsInfo;
// these functions throw on error
void EnsureImplementationTimeFormat(const IFactorsInfo* factors);
void EnsureImplementationTimePresence(const IFactorsInfo* factors, ui32 startFactor);
