#include "opts.h"

namespace NUta {

    TDefaultSmartSplitOpts::TDefaultSmartSplitOpts() {
        CutWWW = true;
        CutZeroDomain = true;
        IgnoreMarks = false;
        NumTransitionsMarkThreshold = 2u;
        PathTokenMinLen = 3u;
        MinParamValueLen = 3u;
        IgnoreParamName = true;
        SplitHostWords = false;
        JoinHostSmallTokens = true;
        TryUntranslit = true;
        DoSearchSplit = true;
        SplitPerformThreshold = 1u;
        MinSubTokenLen = 2u;
        IgnoreEmptySplit = true;
        PenalizeForShortSplits = false;
        MinResultWordLen = 1u;
    }

    TFastStrictOpts::TFastStrictOpts() {
        CutWWW = true;
        CutZeroDomain = true;
        IgnoreMarks = false;
        NumTransitionsMarkThreshold = 2;
        PathTokenMinLen = 3;
        MinParamValueLen = 3;
        IgnoreParamName = true;
        SplitHostWords = false;
        JoinHostSmallTokens = true;
        TryUntranslit = false;
        DoSearchSplit = false;
        SplitPerformThreshold = 1;
        MinSubTokenLen = 1;
        IgnoreEmptySplit = true;
        PenalizeForShortSplits = false;
        MinResultWordLen = 1;
    }

    TLightExperimentalOpts::TLightExperimentalOpts() {
        CutWWW = false;
        CutZeroDomain = false;
        IgnoreMarks = false;
        NumTransitionsMarkThreshold = 3;
        PathTokenMinLen = 2;
        MinParamValueLen = 2;
        IgnoreParamName = false;
        JoinHostSmallTokens = false;
        TryUntranslit = false;
        DoSearchSplit = false;
        MinResultWordLen = 1;
        IgnoreScriptNames = false;
        ProcessWinCode = true;
    }

}
