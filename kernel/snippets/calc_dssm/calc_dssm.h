#pragma once

#include <util/generic/string.h>

namespace NSnippets {
    class TConfig;
    class TSentsMatchInfo;
    class TSnip;
    struct ISnippetsCallback;

    float CalcDssmFactor(TString &&query, TString &&snippet, const TConfig &cfg);
    void AdjustCandidatesDssm(TSnip& snip, const  TConfig& cfg, const TSentsMatchInfo& sentsMatchInfo, ISnippetsCallback& callback);
};
