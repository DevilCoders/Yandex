#pragma once
#include <kernel/querydata/saas/qd_saas_key.h>
#include <kernel/saas_trie/idl/trie_key.h>

#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NSc {
    class TValue;
}

namespace NQueryDataSaaS {
    class TSaaSRequestRec;
}

namespace NQuerySearch {
    class TSaaSRequestStats {
    public:
        struct TKeyStat {
            ui32 KeysToSearchTotal = 0;
            ui32 KeysToSendTotal = 0;
            ui32 ReqsTotal = 0;
        };

        TMap<TString, TKeyStat> KeyStats;
    };

    struct TSaaSKeyTypeOpts {
        NSaasTrie::TRealmPrefixSet PrefixSet;
        bool Disable = false;
        bool LastRealmUnique = false;
        bool NormalSearch = true;
        bool UrlMaskSearch = false;
    };

    class TSaaSServiceOpts {
    public:
        using TRef = TIntrusivePtr<TSaaSServiceOpts>;

    public:
        auto GetMaxKeysInRequest() const {
            return MaxKeysInRequest;
        }

        auto GetMaxRequests() const {
            return MaxRequests;
        }

        auto GetDefaultKps() const {
            return DefaultKps;
        }

        const TSaaSKeyTypeOpts& GetKeyOpts(const NQueryDataSaaS::TSaaSKeyType& kt) const {
            return KeyTypes.at(kt);
        }

        auto GetSaaSType() const {
            return SaaSType;
        }

        TVector<NQueryDataSaaS::TSaaSKeyType> GetKeyTypes() const;

        bool IsDisabled() const;

    public:
        void InitFromJsonThrow(TStringBuf);

        void InitFromSchemeThrow(const NSc::TValue&);

        NSc::TValue AsScheme() const;

        TString AsJson() const;

    private:
        TMap<NQueryDataSaaS::TSaaSKeyType, TSaaSKeyTypeOpts> KeyTypesOriginal;
        TMap<NQueryDataSaaS::TSaaSKeyType, TSaaSKeyTypeOpts> KeyTypes;
        ui64 DefaultKps = 1;
        ui32 MaxKeysInRequest = 32;
        ui32 MaxRequests = 32;
        NQueryDataSaaS::EQDSaaSType SaaSType = NQueryDataSaaS::EQDSaaSType::None;
        bool OptimizeKeyTypes = false;
    };
}
