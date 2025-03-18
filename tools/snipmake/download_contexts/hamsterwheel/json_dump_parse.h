#pragma once

#include <util/generic/string.h>
#include <util/generic/hash.h>

using TRequestBySource = THashMap<TString, TString>;

bool ParseEventlogFromJsonDump(const TString& jsonDump, TRequestBySource& result);
