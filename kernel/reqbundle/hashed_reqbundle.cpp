#include "hashed_reqbundle.h"

namespace NReqBundle {
namespace NDetail {
    void THashedReqBundleAdapter::Rehash()
    {
        for (auto& request : NDetail::BackdoorAccess(*this).Requests) {
            HashToRequest[Ser.GetBinaryHash(*request, true)] = request;
        }
    }

    void THashedReqBundleAdapter::AddRequestUnsafe(const TRequestPtr& request, THashValue hash)
    {
        if (Options.DuplicatesResolver) {
            for (const auto& entry : request->GetFacets().GetEntries()) {
                request->Facets().Set(entry.GetId(), Options.DuplicatesResolver->GetNormalizedValue(entry.GetValue(), entry.GetId()));
            }
        }
        NDetail::BackdoorAccess(*this).Requests.push_back(request);
        HashToRequest[hash] = request; // can overwrite in case of collision
    }

    bool THashedReqBundleAdapter::FindRequestByHash(THashValue hash, TRequestPtr& request) const
    {
        auto it = HashToRequest.find(hash);
        if (it != HashToRequest.end()) {
            request = it->second;
            return true;
        }
        return false;
    }

    bool THashedReqBundleAdapter::FindRequest(TConstRequestAcc reference, TRequestPtr& request) const
    {
        return FindRequestByHash(Ser.GetBinaryHash(reference, true), request);
    }

    TRequestPtr THashedReqBundleAdapter::AddRequest(const TRequestPtr& request)
    {
        THashValue hash = Ser.GetBinaryHash(*request, true);

        TRequestPtr res;
        if (FindRequestByHash(hash, res)) {
            bool collision = false;
            for (auto entry : request->GetFacets().GetEntries()) {
                float value = 0.0f;

                if (res->GetFacets().Lookup(entry.GetId(), value)) {
                    collision = true; // There are duplicates in reqbundle
                } else {
                    value = entry.GetValue();
                }

                if (Options.DuplicatesResolver) {
                    if (collision) {
                        value = Options.DuplicatesResolver->GetUpdatedValue(value, entry.GetValue(), entry.GetId());
                    } else {
                        value = Options.DuplicatesResolver->GetNormalizedValue(entry.GetValue(), entry.GetId());
                    }
                    res->Facets().Set(entry.GetId(), value);
                }
            }

            if (Options.DuplicatesResolver) {
                return res;
            } else if (!collision) {
                for (auto entry : request->GetFacets().GetEntries()) {
                    res->Facets().Set(entry.GetId(), entry.GetValue());
                }

                return res;
            }
        }

        AddRequestUnsafe(request, hash);
        return request;
    }
} // NDetail
} // NReqBundle
