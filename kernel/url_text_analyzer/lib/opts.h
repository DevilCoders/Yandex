#pragma once

#include <util/system/types.h>

namespace NUta {

    struct TOpts {
        bool CutWWW = false;
        bool CutZeroDomain = false;
        bool IgnoreMarks = false;
        bool IgnoreNumbers = true;
        ui32 NumTransitionsMarkThreshold = ui32(-1);
        ui32 PathTokenMinLen = 1u;
        ui32 MinParamValueLen = 1u;
        bool IgnoreParamName = false;
        bool SplitHostWords = false;
        bool JoinHostSmallTokens = false;
        bool TryUntranslit = false;
        bool DoSearchSplit = false;
        ui32 SplitPerformThreshold = 1u;
        ui32 MinSubTokenLen = 1u;
        bool IgnoreEmptySplit = true;
        bool PenalizeForShortSplits = false;
        ui32 MinResultWordLen = 1u;
        bool IgnoreScriptNames = true;
        bool ProcessWinCode = false;
        ui32 MaxTranslitCandidates = 3; //-1 for no limit
        size_t CacheSize = 0; //0 for no cache
        size_t BucketNum = 0; //cache buckets

        virtual ~TOpts() = default;
    };

    struct TDefaultSmartSplitOpts : public TOpts {
        TDefaultSmartSplitOpts();
    };

    struct TFastStrictOpts : public TOpts {
        TFastStrictOpts();
    };

    struct TLightExperimentalOpts : public TOpts {
        TLightExperimentalOpts();
    };

} // NUta
