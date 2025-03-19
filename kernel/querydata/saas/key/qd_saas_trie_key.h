#pragma once

#include "qd_saas_key.h"

#include <util/digest/city.h>
#include <util/generic/strbuf.h>

#include <utility>

namespace NQueryDataSaaS {
    const TString& SubkeyTypeForTrie(ESaaSSubkeyType);

    ESaaSSubkeyType ParseTrieSubkeyType(TStringBuf);

    class TSaaSTrieKey {
    public:
        static constexpr char Delimiter = '\t';

        const auto& GetKey() const {
            return Key;
        }

        const auto& GetPrefix() const {
            return Prefix;
        }

        TSaaSTrieKey& Append(const TSaaSTrieKey&);

        TSaaSTrieKey& Append(ESaaSSubkeyType, TStringBuf);

        TString AsString() const;

        void AsString(TString& s) const {
            s.append(Key);
        }

        size_t Hash() const {
            return CityHash64(Key);
        }

        bool operator==(const TSaaSTrieKey& key) const {
            return std::tie(Prefix, Key) == std::tie(key.Prefix, key.Key);
        }

        bool operator<(const TSaaSTrieKey& key) const {
            return std::tie(Prefix, Key) < std::tie(key.Prefix, key.Key);
        }

        static TSaaSTrieKey From(ESaaSSubkeyType sst, TStringBuf sk);

        static TSaaSTrieKey From(const TSaaSSubkey& sk);

        static TSaaSTrieKey From(const TSaaSKey& key);

        static TSaaSTrieKey Parse(TStringBuf);

        static void GenerateMainKey(TString& mKey, const TSaaSKeyType& kt);

        static void GenerateRealmKey(TString& rKey, TStringBuf key);

    private:
        TString Key;
        TString Prefix;
    };
}

template<>
struct THash<NQueryDataSaaS::TSaaSTrieKey> {
    size_t operator() (NQueryDataSaaS::TSaaSTrieKey k) {
        return k.Hash();
    }
};
