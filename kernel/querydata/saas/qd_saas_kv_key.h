#pragma once

#include "qd_saas_key.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/generic/vector.h>

#include <util/digest/numeric.h>

#include <utility>

namespace NQueryDataSaaS {
    extern const size_t HASH128_HEX_SIZE;

    class TSaaSKVKey {
    public:
        using THash128 = std::pair<ui64, ui64>;

    public:
        TSaaSKVKey() = default;

        explicit TSaaSKVKey(THash128 hash)
            : Hash128(hash)
        {}

        explicit TSaaSKVKey(ui64 a, ui64 b)
            : Hash128(a, b)
        {}

        THash128 GetHash128() const {
            return Hash128;
        }

        TSaaSKVKey& Append(TSaaSKVKey o);

        TSaaSKVKey& Append(ESaaSSubkeyType sst, TStringBuf sk) {
            return Append(From(sst, sk));
        }

        TString AsString() const;

        void AsString(TString&) const;

        size_t Hash() const {
            return CombineHashes(Hash128.first, Hash128.second);
        }

        bool operator==(const TSaaSKVKey& key) const {
            return Hash128 == key.Hash128;
        }

        bool operator<(const TSaaSKVKey& key) const {
            return Hash128 < key.Hash128;
        }

        static TSaaSKVKey From(const TSaaSSubkey& sk);

        static TSaaSKVKey From(ESaaSSubkeyType sst, TStringBuf sk);

        static TSaaSKVKey From(const TSaaSKey& k);

        static TSaaSKVKey Parse(TStringBuf sk);

    private:
        THash128 Hash128;
    };
}

template<>
struct THash<NQueryDataSaaS::TSaaSKVKey> {
    size_t operator() (NQueryDataSaaS::TSaaSKVKey k) {
        return k.Hash();
    }
};

