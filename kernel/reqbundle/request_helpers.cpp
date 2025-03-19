#include "request_helpers.h"
#include "serializer.h"

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/utility.h>


namespace {
    struct THelpers { // used PerformMatching method structures
        struct TQueryWordSubTokenPosition {
            ui32 WordId = 0;
            ui32 SubTokenId = 0;

            auto AsTuple() const {
                return std::forward_as_tuple(WordId, SubTokenId);
            }

            bool operator<=(const TQueryWordSubTokenPosition& b) const {
                return AsTuple() <= b.AsTuple();
            }

            bool operator<(const TQueryWordSubTokenPosition& b) const {
                return AsTuple() < b.AsTuple();
            }
        };

        struct TListOfPositions {
            static constexpr size_t ObjectsInCacheLine = 64 / sizeof(TQueryWordSubTokenPosition);
            TStackVec<TQueryWordSubTokenPosition, ObjectsInCacheLine> Positions;
            ui32 CurrentOffset = 0;

            void Append(ui32 wordId, ui32 subTokenId) {
                TQueryWordSubTokenPosition obj{wordId, subTokenId};
                Y_ASSERT(Positions.empty() || Positions.back() < obj);
                Positions.push_back(obj);
            }

            TMaybe<TQueryWordSubTokenPosition> FetchNextMatching(TMaybe<TQueryWordSubTokenPosition> lastMatched) {
                TMaybe<TQueryWordSubTokenPosition> result;
                if (lastMatched) {
                    //NOTE: bin-search possible
                    while (CurrentOffset < Positions.size() && Positions[CurrentOffset] <= *lastMatched) {
                        ++CurrentOffset;
                    }
                }
                if (CurrentOffset < Positions.size()) {
                    result = Positions[CurrentOffset];
                    CurrentOffset += 1;
                }
                return result;
            }
        };
    };

    void RemapMatches(
        TVector<NReqBundle::NDetail::TMatchData>& matches,
        const TVector<std::pair<size_t, size_t>>& matchingRanges)
    {
        for (auto& match : matches) {
            match.WordIndexFirst = matchingRanges[match.WordIndexFirst].first;
            match.WordIndexLast = matchingRanges[match.WordIndexLast].second;
        }
    }

    TVector<bool> WordHasSynonyms(NReqBundle::TConstRequestAcc request) {
        TVector<bool> result(request.GetNumWords(), false);
        for (const auto& match : request.GetMatches()) {
            if (NReqBundle::IsMatchFromThesaurus(match.GetSynonymMask()) && match.GetType() == NReqBundle::EMatchType::Synonym) {
                for (size_t i : xrange(match.GetWordIndexFirst(), match.GetWordIndexLast() + 1)) {
                    result[i] = true;
                }
            }
        }
        return result;
    }

    TVector<bool> HasConstraintsBlock(NReqBundle::TConstRequestAcc request, NReqBundle::TConstReqBundleAcc bundle) {
        TVector<bool> result(request.GetNumWords(), false);
        TSet<size_t> constraintsBlockIdx;
        for (const auto& constraint : bundle.GetConstraints()) {
            constraintsBlockIdx.insert(constraint.GetBlockIndices().begin(), constraint.GetBlockIndices().end());
        }
        for (const auto& match : request.GetMatches()) {
            if (constraintsBlockIdx.contains(match.GetBlockIndex())) {
                for (size_t i : xrange(match.GetWordIndexFirst(), match.GetWordIndexLast() + 1)) {
                    result[i] = true;
                }
            }
        }
        return result;
    }

    bool NeedAlterOriginalRequest(NReqBundle::TRequestAcc request, const NReqBundle::TMergeSynonymsOptions& options) {
        if (options.Condition == NReqBundle::EMergeCondition::Always) {
            return true;
        }
        TVector<bool> wordHasSynonyms = WordHasSynonyms(request);
        if (options.Scope == NReqBundle::EMergeScope::ByRequest) {
            return Count(wordHasSynonyms, true) == 0;
        } else {
            return Count(wordHasSynonyms, false) > 0;;
        }
    }
} // namespace


namespace NReqBundle {
    TReqBundleSubsetPtr RemoveOriginal(TConstReqBundleAcc bundle) {
        TAllRestrictOptions restricts;
        restricts.Add(TFacetId{TExpansion::OriginalRequest}).SetEnabled(false);
        return RestrictReqBundle(bundle, restricts);
    }

    bool IsMatchFromThesaurus(ui32 synonymMask) {
        return (synonymMask & TE_THESAURUS) != 0;
    }

    void PrepareOriginalRequest(TRequestAcc request, const TVector<bool>& hasConstraints) {
        Y_ASSERT(request.HasExpansionType(TExpansion::OriginalRequest));

        auto& matches = NReqBundle::NDetail::BackdoorAccess(request).Matches;
        TSet<size_t> matchesToDelete;
        for (size_t m : xrange(matches.size())) {
            const auto& match = matches[m];
            bool hasConstraintsPart = Count(
                hasConstraints.begin() + match.WordIndexFirst,
                hasConstraints.begin() + match.WordIndexLast + 1, true) > 0;
            if (IsMatchFromThesaurus(match.SynonymMask) && !hasConstraintsPart) {
                matchesToDelete.insert(m);
            }
        }
        RemoveRequestMatches(NReqBundle::NDetail::BackdoorAccess(request), matchesToDelete);
    }

    TVector<TString> ExtractOriginalRequestWords(
        TConstRequestAcc request,
        TSequenceAcc seq)
    {
        TVector<TString> words;
        words.resize(request.GetNumWords());

        NSer::TDeserializer deser;

        for (auto match : request.GetMatches()) {
            if (match.GetType() == TMatch::OriginalWord) {
                for (size_t wordIndex : match.GetWordIndexes()) {
                    auto elem = seq.Elem(match);

                    bool unpacked = false;
                    if (!elem.HasBlock()) {
                        elem.PrepareBlock(deser);
                        unpacked = true;
                    }

                    if (elem.HasBlock() && elem.GetBlock().GetNumWords() >= 1) {
                        auto block = seq.GetElem(match).GetBlock();
                        words[wordIndex] = block.GetWord().GetText();
                    }
                    if (unpacked) {
                        elem.DiscardBlock();
                    }
                }
            }
        }

        return words;
    }

    TVector<TString> ExtractRequestWords(
        TConstRequestAcc request)
    {
        TVector<TString> words;
        words.reserve(request.GetNumWords());

        NSer::TDeserializer deser;

        for (size_t i : xrange(request.GetNumWords())) {
            words.push_back(request.GetWord(i).GetTokenText());
        }

        return words;
    }

    TVector<std::pair<size_t, size_t>> GetMatchingRanges(
        const TVector<TString>& wordsToMatch,
        const TVector<TString>& mainWords)
    {

        TMemoryPool pool(1024);

        THashMap<
            TWtringBuf,
            THelpers::TListOfPositions,
            THash<TWtringBuf>,
            TEqualTo<TWtringBuf>,
            TPoolAlloc<THelpers::TListOfPositions>> positions(&pool);

        TUtf16String wideBuffer;

        {//Prepare: build subtokens mapping via new tokenization of query-words
            ui32 wordId = 0;
            ui32 subtokenId = 0;

            auto subtokensHandler = MakeCallbackTokenHandler([&] (const TWideToken& token, size_t origleng, NLP_TYPE type) {
                Y_UNUSED(origleng, type);
                if (type == NLP_WORD || type == NLP_MARK || type == NLP_INTEGER || type == NLP_FLOAT) {
                    for (const TCharSpan * it = token.SubTokens.begin(); it != token.SubTokens.end(); ++it) {
                        TWtringBuf subtoken(token.Token, it->Pos, it->Len);
                        TWtringBuf ownedSubtoken = pool.AppendString(subtoken);

                        positions[ownedSubtoken].Append(wordId, subtokenId);

                        subtokenId += 1;
                    }
                }
            });

            TNlpTokenizer tokenizer(subtokensHandler, false);
            for (TStringBuf word : mainWords) {
                if (!word.empty()) {
                    subtokenId = 0;
                    UTF8ToWide(word, wideBuffer);
                    wideBuffer.to_lower(); //NOTE: actually - can be skipped
                    tokenizer.Tokenize(wideBuffer);
                }
                wordId += 1;
            }
        }

        TVector<std::pair<size_t, size_t>> wordMatching(wordsToMatch.size(), {mainWords.size(), 0});

        {
            TMaybe<THelpers::TQueryWordSubTokenPosition> lastMatching;
            char asciiSpace = ' ';

            for (size_t i  : xrange(wordsToMatch.size())) {//iterate over words
                TStringBuf wholeInfo = wordsToMatch[i];

                while (TStringBuf token = wholeInfo.NextTok(asciiSpace)) {//iterator over subtokens
                    TWtringBuf wideToken = UTF8ToWide(token, wideBuffer);

                    TMaybe<THelpers::TQueryWordSubTokenPosition> matchRes;
                    if (auto ptr = positions.FindPtr(wideToken)) {
                        matchRes = ptr->FetchNextMatching(lastMatching);
                    }

                    if (matchRes) {
                        lastMatching = matchRes;
                        wordMatching[i].first = Min<size_t>(wordMatching[i].first, matchRes->WordId);
                        wordMatching[i].second = Max<size_t>(wordMatching[i].second, matchRes->WordId);
                    }
                }
            }
        }

        return wordMatching;
    }

    TReqBundlePtr MergeSynonymsInBundle(TConstReqBundleAcc bundleData, const TMergeSynonymsOptions& options) {
        TReqBundlePtr bundle = new TReqBundle(bundleData);

        if (bundle->GetNumRequests() == 0) {
            return bundle;
        }

        size_t originalIndex = Max<size_t>();
        size_t synonymsIndex = Max<size_t>();

        for (size_t i : xrange(bundle->GetNumRequests())) {
            TRequestPtr request = bundle->GetRequestPtr(i);
            if (request->HasExpansionType(TExpansion::OriginalRequest)) {
                Y_ASSERT(originalIndex ==  Max<size_t>());
                originalIndex = i;
            } else if (request->HasExpansionType(TExpansion::OriginalRequestSynonyms)) {
                Y_ASSERT(synonymsIndex == Max<size_t>());
                synonymsIndex = i;
            }
        }

        if (Max<size_t>() == originalIndex || Max<size_t>() == synonymsIndex) {
            return RestrictReqBundleByExpansions(*bundle, {TExpansion::OriginalRequest})->GetResult();
        }

        TRequestPtr origRequest = bundle->GetRequestPtr(originalIndex);
        if (!NeedAlterOriginalRequest(*origRequest, options)) {
            return RestrictReqBundleByExpansions(*bundle, {TExpansion::OriginalRequest})->GetResult();
        }

        TVector<bool> hasConstraints = HasConstraintsBlock(*origRequest, *bundle);
        if (options.Condition == EMergeCondition::Always) {
            PrepareOriginalRequest(*origRequest, hasConstraints);
        }

        TVector<NDetail::TMatchData> matchesToSave;
        TVector<EFormClass> bestFormClasses;
        TRequestPtr synonymsRequest = bundle->GetRequestPtr(synonymsIndex);

        Y_ENSURE(synonymsRequest->GetTrCompatibilityInfo().Defined());

        auto& synonymsMatches = NDetail::BackdoorAccess(*synonymsRequest).Matches;

        if (origRequest->GetTrCompatibilityInfo()->WordCount != synonymsRequest->GetTrCompatibilityInfo()->WordCount) {
            if (!options.AlignTokens) {
                return RestrictReqBundleByExpansions(*bundle, {TExpansion::OriginalRequest})->GetResult();
            }

            TVector<TString> wordsToMatch = ExtractRequestWords(*synonymsRequest);
            TVector<TString> mainWords = ExtractOriginalRequestWords(*origRequest, bundle->Sequence());
            TVector<std::pair<size_t, size_t>> wordMatching = GetMatchingRanges(wordsToMatch, mainWords);
            RemapMatches(synonymsMatches, wordMatching);
        }

        TVector<bool> wordHasSynonyms = WordHasSynonyms(*origRequest);
        size_t numSynonyms = 0;
        for (size_t m : xrange(synonymsMatches.size())) {
            const auto& match = synonymsMatches[m];
            const auto& info = synonymsRequest->GetTrCompatibilityInfo();

            if (match.Type == EMatchType::Synonym && match.WordIndexFirst <= match.WordIndexLast) {
                bool hasUncoveredSynonyms = Count(wordHasSynonyms.begin() + match.WordIndexFirst, wordHasSynonyms.begin() + match.WordIndexLast + 1, false) > 0;
                bool hasConstraintsPart = Count(hasConstraints.begin() + match.WordIndexFirst, hasConstraints.begin() + match.WordIndexLast + 1, true) > 0;
                if (!hasConstraintsPart && (options.Condition == EMergeCondition::Always
                    || options.Scope == EMergeScope::ByRequest
                    || hasUncoveredSynonyms))
                {
                    matchesToSave.emplace_back(match);
                    bestFormClasses.emplace_back(info->MarkupPartsBestFormClasses[numSynonyms]);
                    ++numSynonyms;
                }
            }
        }
        AppendSynonymMatches(NReqBundle::NDetail::BackdoorAccess(*origRequest), matchesToSave, bestFormClasses);

        return RestrictReqBundleByExpansions(*bundle, {TExpansion::OriginalRequest})->GetResult();
    }

    // This function throws exceptions, so it is pretty dangerous)
    TReqBundlePtr MergeSynonymsToReqBundle(
        TConstReqBundleAcc bundle,
        TConstReqBundleAcc synonyms,
        const TMergeSynonymsOptions& options)
    {
        TReqBundleSubsetPtr synonymsReqSubBundle = RestrictReqBundleByExpansions(synonyms, {TExpansion::OriginalRequestSynonyms});
        TReqBundleSubsetPtr originalReqSubBundle = RestrictReqBundleByExpansions(bundle, {TExpansion::OriginalRequest});
        TReqBundleSubsetPtr otherReqSubBundle = RemoveOriginal(bundle);

        TReqBundleMerger merger;
        merger.AddBundle(*synonymsReqSubBundle->GetResult());
        merger.AddBundle(*originalReqSubBundle->GetResult());
        TReqBundlePtr mergerResult = merger.GetResult();

        TReqBundleMerger finalMerger;
        finalMerger.AddBundle(*MergeSynonymsInBundle(*mergerResult, options));
        finalMerger.AddBundle(*otherReqSubBundle->GetResult());
        return finalMerger.GetResult();
    }


    void AppendCountryIntoOriginal(TReqBundlePtr in) {
        Y_ENSURE(in);
        TRequestPtr original = nullptr;
        TRequestPtr rwc = nullptr;

        auto& data = NDetail::BackdoorAccess(*in);
        for (TRequestPtr ptr: data.Requests) {
            if (ptr->HasExpansionType(NLingBoost::EExpansionType::OriginalRequest)) {
                original = ptr;
            }
            if (ptr->HasExpansionType(NLingBoost::EExpansionType::RequestWithCountryName)) {
                rwc = ptr;
            }
        }

        if (!original || !rwc || rwc->GetNumWords() <= original->GetNumWords()) {
            return;
        }

        //NOTE: here is no division by facet, append happens into all requests that stay together with original-request
        size_t firstAddedWordId = original->GetNumWords();

        TVector<NDetail::TMatchData> resultMatches;
        for (size_t id : xrange(firstAddedWordId)) {
            resultMatches.push_back(NDetail::BackdoorAccess(*original).Matches[id]);
            Y_ASSERT(resultMatches.back().Type == EMatchType::OriginalWord);
        }
        for (size_t id : xrange(firstAddedWordId, rwc->GetNumWords())) {
            NDetail::BackdoorAccess(*original).Words.push_back(NDetail::BackdoorAccess(*rwc).Words[id]);
            if (firstAddedWordId > 0) {
                NDetail::BackdoorAccess(*original).Proxes.push_back(NDetail::BackdoorAccess(*rwc).Proxes[id - 1]);
            }

            if (NDetail::BackdoorAccess(*original).TrCompatibilityInfo) {
                NDetail::BackdoorAccess(*original).TrCompatibilityInfo->MainPartsWordMasks.push_back(EMatchType::OriginalWord);
                NDetail::BackdoorAccess(*original).TrCompatibilityInfo->WordCount += 1;
            }

            resultMatches.push_back(NDetail::BackdoorAccess(*rwc).Matches[id]);
            Y_ASSERT(resultMatches.back().Type == EMatchType::OriginalWord);
        }

        for (size_t id : xrange(firstAddedWordId, NDetail::BackdoorAccess(*original).Matches.size())) {
            resultMatches.push_back(NDetail::BackdoorAccess(*original).Matches[id]);
        }
        NDetail::BackdoorAccess(*original).Matches = std::move(resultMatches);
    }

} // namespace NReqBundle
