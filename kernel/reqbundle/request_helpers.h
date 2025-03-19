#pragma once

#include "accessor.h"
#include "merge.h"
#include "request_contents.h"
#include "restrict.h"

#include <kernel/qtree/richrequest/protos/thesaurus_exttype.pb.h>

#include <util/generic/string.h>
#include <library/cpp/scheme/scheme.h>

namespace NReqBundle {
    TVector<TString> ExtractOriginalRequestWords(TConstRequestAcc request, TSequenceAcc seq);
    TVector<TString> ExtractRequestWords(TConstRequestAcc request);
    bool IsMatchFromThesaurus(ui32 synonymMask);

    TVector<std::pair<size_t, size_t>> GetMatchingRanges(
        const TVector<TString>& wordsToMatch,
        const TVector<TString>& mainWords);

    enum class EMergeScope {
        ByWord,
        ByRequest
    };

    enum class EMergeCondition {
        Always,
        IfEmpty
    };

    struct TMergeSynonymsOptions {
        bool AlignTokens = true;
        EMergeScope Scope = EMergeScope::ByRequest;
        EMergeCondition Condition = EMergeCondition::Always;
    };

    TReqBundlePtr MergeSynonymsToReqBundle(
        TConstReqBundleAcc bundle,
        TConstReqBundleAcc synonyms,
        const TMergeSynonymsOptions& options = {});

    // FORMULA-1744
    // search RequestWithCountryName in input
    // and append blocks into OriginalRequqest
    void AppendCountryIntoOriginal(TReqBundlePtr);
} // namespace NReqBundle
