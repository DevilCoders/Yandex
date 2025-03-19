#pragma once

#include <util/generic/string.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    TString Token;
    TString Endpoint;
    TString Database;
    TString Table;
    size_t ThreadCount = 1;

    void Parse(int argc, char** argv);
};

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
