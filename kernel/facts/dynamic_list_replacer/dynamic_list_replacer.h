#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/types.h>


namespace NFacts {

using THashVector = TVector<ui16>;

struct TMatchResult {
    size_t ListCandidateScore;
    size_t NumberOfHeaderElementsToSkip;
};

size_t FindLongestCommonSlice(const THashVector& a, const THashVector& b, size_t& aToCommon, size_t& bToCommon, size_t minAllowedCommonSliceSize = 1);

size_t MatchFactTextWithListCandidate(const THashVector& factTextHash, const NSc::TDict& listCandidateIndex, size_t& numberOfHeaderElementsToSkip);
TMatchResult MatchFactTextWithListCandidate(const THashVector& factTextHash, const TString& listCandidateIndexJson);

void ConvertListDataToRichFact(NSc::TValue& serpData, const NSc::TDict& listData, size_t numberOfHeaderElementsToSkip);
TString ConvertListDataToRichFact(const TString& serpDataJson, const TString& listDataJson, size_t numberOfHeaderElementsToSkip);

bool TryReplaceTextWithList(NSc::TValue& serpData, const THashVector& factTextHash, const NSc::TArray& listCandidates, i64& candidateDownCounter);

TUtf16String NormalizeTextForIndexer(const TUtf16String& text);
TString NormalizeTextForIndexer(const TString& text);

THashVector DynamicListReplacerHashVector(const TUtf16String& text);
THashVector DynamicListReplacerHashVector(const TString& text);

}  // namespace NFacts
