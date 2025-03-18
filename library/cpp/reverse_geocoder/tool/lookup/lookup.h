#pragma once

#include <util/generic/string.h>

class TLookupTool {
public:
    struct TConfig {
        bool Trace = false;
        TString Path;
    };

    TLookupTool(const TConfig& config)
        : Config_(config)
    {
    }

    int Run(const TString& llpoints_filename);

private:
    TConfig Config_;
};
