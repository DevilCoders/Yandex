#pragma once

#include <kernel/reqbundle/reqbundle.h>

#include <util/generic/map.h>


namespace NTextMachine {
    class TExpansionsResolver {
    public:
        using TIndexes = TSet<size_t>;
        using TMatches = TVector<NReqBundle::TConstMatchAcc>;

        TExpansionsResolver(NReqBundle::TConstReqBundleAcc bundle)
            : Bundle(bundle)
        {}

        const TIndexes& GetExpansionsByBlockId(size_t blockId) {
            if (auto ptr = ExpansionsByBlockId.FindPtr(blockId)) {
                return *ptr;
            }

            TIndexes expansions;
            for (size_t requestIndex : xrange(Bundle.GetNumRequests())) {
                const NReqBundle::TConstRequestAcc request = Bundle.GetRequest(requestIndex);
                for (const NReqBundle::TConstMatchAcc match : request.GetMatches()) {
                    if (match.GetBlockIndex() == blockId) {
                        expansions.insert(requestIndex);
                        break;
                    }
                }
            }

            return (ExpansionsByBlockId[blockId] = expansions);
        }

        TIndexes GetRequestsByExpansionType(NLingBoost::EExpansionType expansionType) const {
            TIndexes requests;
            for (size_t requestIndex : xrange(Bundle.GetNumRequests())) {
                const NReqBundle::TConstRequestAcc request = Bundle.GetRequest(requestIndex);
                for (NReqBundle::TConstFacetEntryAcc entry : request.GetFacets().GetEntries()) {
                    if (entry.GetExpansion() == expansionType) {
                        requests.insert(requestIndex);
                        break;
                    }
                }
            }
            return requests;
        }

    private:
        TMap<size_t, TIndexes> ExpansionsByBlockId;
        NReqBundle::TConstReqBundleAcc Bundle;
    };
} // NTextMachine
