#pragma once

#include "private.h"

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

enum class EClientType
{
    Read,
    Write
};

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    TString FilePath;
    ui64 FileSize;
    ui64 RequestSize;
    EClientType Type;
    TString TypeStr;
    ui64 SleepBetweenWrites;
    ui64 SleepBeforeStart;

    void Parse(int argc, char** argv);
};

}   // namespace NCloud::NFileStore
