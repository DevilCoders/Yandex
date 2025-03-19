#pragma once

#include "block_contents.h"
#include "facets.h"
#include "request_tr_compatibility_info.h"

#include <kernel/lingboost/freq.h>
#include <kernel/lingboost/constants.h>

#include <util/generic/maybe.h>

namespace NReqBundle {
namespace NDetail {
    struct TProxData {
        float Cohesion = 0.0;
        bool Multitoken = false;
    };

    struct TRequestWordData {
        NLingBoost::TRevFreq RevFreq;
        NDetail::TText Token;
    };

    using TAnchorWordsRange = TMaybe<std::pair<ui32, ui32>>;

    struct TMatchData {
        EMatchType Type = NLingBoost::TMatch::OriginalWord;
        NLingBoost::TRevFreq RevFreq;
        ui32 SynonymMask = 0x0;
        double Weight = 1.0f; // Make it float. Double is to avoid diffs now.

        size_t BlockIndex = 0;
        size_t WordIndexFirst = 0;
        size_t WordIndexLast = 0;
        TAnchorWordsRange AnchorWordsRange;
    };

    struct TRequestData {
        TVector<TRequestWordData> Words;
        TVector<TProxData> Proxes;
        TVector<TMatchData> Matches;
        TFacetsData Facets;
        TMaybe<TRequestTrCompatibilityInfo> TrCompatibilityInfo;
        ui32 AnchorsSrcLength = 0;
    };

    void InitRequestData(TRequestData& data, size_t numWords);

    void RemoveRequestMatches(
        TRequestData& data,
        const TSet<size_t>& matchIndices);

    void AppendSynonymMatches(
        TRequestData& data,
        const TVector<TMatchData>& matches,
        const TVector<EFormClass>& bestFormClasses = {});
} // NDetail
} // NReqBundle
