#pragma once

#include "duplicates_resolvers.h"
#include "hashed_sequence.h"
#include "reqbundle.h"
#include "serializer.h"

namespace NReqBundle {
namespace NDetail {
    class THashedReqBundleAdapter
        : public TReqBundleAcc
    {
    public:
        struct TOptions {
            const IDuplicatesResolver* DuplicatesResolver = nullptr;

            TOptions() {}
        };

    private:
        using THashValue = ui64;
        using TRequestHash = THashMap<ui64, TRequestPtr>;

    private:
        TOptions Options;
        TRequestHash HashToRequest;
        TReqBundleSerializer Ser{TCompressorFactory::NO_COMPRESSION};

    public:
        THashedReqBundleAdapter(TReqBundleAcc bundle, const TOptions& options = TOptions{})
            : TReqBundleAcc(bundle)
            , Options(options)
        {
            Rehash();
        }

        TReqBundleAcc UnhashedReqBundle() const {
            return *this;
        }

        TRequestPtr AddRequest(const TRequestPtr& request);
        bool FindRequest(TConstRequestAcc reference, TRequestPtr& request) const;

    private:
        void AddRequestUnsafe(const TRequestPtr& request, THashValue hash);
        bool FindRequestByHash(THashValue hash, TRequestPtr& request) const;
        void Rehash();
    };
} // NDetail
} // NReqBundle
