#pragma once

#include <util/draft/date.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NSqueezeRunner {

enum class ESqueezeRunMode {
    YT /* "yt" */,
    LOCAL /* "local" */,
};

struct TCliOpts {
    ESqueezeRunMode RunMode;
    TDate Date;
    TString BlockstatFile;
    TString DstFolder;
    TString LowerKey;
    TString OutputFile;
    TString Pool;
    TString Server;
    TString SqueezeParamsFile;
    TString UpperKey;
    TString UserSessionsFile;
    TVector<TString> Services;
    TVector<TString> Testids;
    ui32 DataSizePerJobGB = 4;
    ui32 MemoryLimitGB = 4;
    ui32 MaxDataSizePerJobGB = 200;
};

TCliOpts ParseOptions(int argc, const char** argv);

};
