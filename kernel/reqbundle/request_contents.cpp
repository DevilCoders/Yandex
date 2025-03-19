#include "request_contents.h"

#include <util/generic/algorithm.h>


namespace NReqBundle {
namespace NDetail {
    void InitRequestData(TRequestData& data, size_t numWords) {
        data.Words.resize(numWords);
        data.Proxes.resize(numWords > 0 ? numWords - 1 : 0);
    }

    void RemoveRequestMatches(TRequestData& data, const TSet<size_t>& matchIndices) {
        TVector<TMatchData> newMatches;
        newMatches.reserve(data.Matches.size());

        TMaybe<TRequestTrCompatibilityInfo> newTrInfo;
        if (data.TrCompatibilityInfo.Defined()) {
            newTrInfo.ConstructInPlace();
            newTrInfo->WordCount = data.TrCompatibilityInfo->WordCount;
            newTrInfo->MainPartsWordMasks.reserve(data.TrCompatibilityInfo->MainPartsWordMasks.size());
            newTrInfo->MarkupPartsBestFormClasses.reserve(data.TrCompatibilityInfo->MarkupPartsBestFormClasses.size());
            newTrInfo->TopAndArgsForWeb = data.TrCompatibilityInfo->TopAndArgsForWeb;
        }

        size_t trMainPartIndex = 0;
        size_t trMarkupPartIndex = 0;

        for (size_t index : xrange(data.Matches.size())) {
            if (matchIndices.contains(index)) {
                continue;
            }
            newMatches.emplace_back(
                std::move(data.Matches[index])
            );

            if (newTrInfo) {
                switch (data.Matches[index].Type) {
                    case TMatch::OriginalWord: {
                        newTrInfo->MainPartsWordMasks.push_back(
                            data.TrCompatibilityInfo->MainPartsWordMasks[trMainPartIndex]);

                        trMainPartIndex += 1;
                        break;
                    }
                    case TMatch::Synonym: {
                        newTrInfo->MarkupPartsBestFormClasses.push_back(
                            data.TrCompatibilityInfo->MarkupPartsBestFormClasses[trMarkupPartIndex]);

                        trMarkupPartIndex += 1;
                        break;
                    }
                    default: {
                        Y_ASSERT(false);
                        break;
                    }
                }
            }
        }

        data.Matches.swap(newMatches);
        data.TrCompatibilityInfo.swap(newTrInfo);
    }

    void AppendSynonymMatches(
        TRequestData& data,
        const TVector<TMatchData>& matches,
        const TVector<EFormClass>& bestFormClasses)
    {
        Y_ASSERT(bestFormClasses.empty() || matches.size() == bestFormClasses.size());

        data.Matches.insert(
            data.Matches.end(),
            matches.begin(),
            matches.end());

        for (const TMatchData& match : matches) {
            Y_ASSERT(match.Type == TMatch::Synonym);
            Y_ASSERT(match.WordIndexFirst < data.Words.size());
            Y_ASSERT(match.WordIndexLast < data.Words.size());
        }

        if (data.TrCompatibilityInfo.Defined()) {
            data.TrCompatibilityInfo->MarkupPartsBestFormClasses.insert(
                data.TrCompatibilityInfo->MarkupPartsBestFormClasses.end(),
                bestFormClasses.begin(),
                bestFormClasses.end());
        }
    }
} // NDetail
} // NReqBundle
