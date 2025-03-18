#pragma once

#include <util/system/defaults.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NMinHash {
    void CreateHash(const TString& serverName, const TString& tableName, ui32 numPortions, double loadFactor, ui32 keysPerBucket, ui8 fprSize, bool wide, const TString& fileName);
    void CreateTable(const TString& serverName, const TString& tableName, const TString& fileName);

}
