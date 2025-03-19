#include "qd_saas_key.h"

#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/split.h>

namespace NQueryDataSaaS {

    const TVector<ESaaSSubkeyType>& GetAllInKeySubkeys() {
        static const TVector<ESaaSSubkeyType> keys = ([]() {
            TVector<ESaaSSubkeyType> res;
            for (int i = SST_INVALID + 1; i < SST_IN_KEY_COUNT; ++i) {
                res.emplace_back((ESaaSSubkeyType)i);
            }
            return res;
        })();
        return keys;
    }

    const TVector<ESaaSSubkeyType>& GetAllInValueSubkeys() {
        static const TVector<ESaaSSubkeyType> keys = ([]() {
            TVector<ESaaSSubkeyType> res;
            for (int i = SST_IN_VALUE + 1; i < SST_IN_VALUE_COUNT; ++i) {
                res.emplace_back((ESaaSSubkeyType)i);
            }
            return res;
        })();
        return keys;
    }

    static auto DoGetTrieKeyOrderForSharding(ESaaSSubkeyType sst) {
        static const size_t order[] {
            /* SST_INVALID */                   Max<size_t>(),
            /* SST_IN_KEY_QUERY_STRONG */       1,
            /* SST_IN_KEY_QUERY_DOPPEL */       1,
            /* SST_IN_KEY_USER_ID */            1,
            /* SST_IN_KEY_USER_LOGIN_HASH */    1,
            /* SST_IN_KEY_USER_REGION */        3,
            /* SST_IN_KEY_STRUCT_KEY */         2,
            /* SST_IN_KEY_URL */                0,
            /* SST_IN_KEY_OWNER  */             0,
            /* SST_IN_KEY_URL_MASK */           0,
            /* SST_IN_KEY_ZDOCID */             0,
            /* SST_IN_KEY_QUERY_DOPPEL_TOKEN */ 1,
            /* SST_IN_KEY_QUERY_DOPPEL_PAIR */  1,
            /* SST_IN_KEY_URL_NO_CGI */  0,
        };
        static_assert(Y_ARRAY_SIZE(order) == SST_IN_KEY_COUNT, "forgot to insert a new key here");
        Y_ENSURE(SubkeyTypeIsValidForKey(sst), "invalid subkey " << (int)sst);
        return std::make_pair(order[sst], sst);
    }

    ESaaSSubkeyType GetTrieKeyForSharding(const TSaaSKeyType& keyType) {
        Y_ENSURE(keyType && KeyTypeIsValid(keyType), "invalid key type " << keyType);
        return *MinElementBy(keyType.begin(), keyType.end(), DoGetTrieKeyOrderForSharding);
    }

    bool IsSplittable(ESaaSSubkeyType sst) {
        static const bool splittable[] {
            /* SST_INVALID */                   0,
            /* SST_IN_KEY_QUERY_STRONG */       0,
            /* SST_IN_KEY_QUERY_DOPPEL */       0,
            /* SST_IN_KEY_USER_ID */            0,
            /* SST_IN_KEY_USER_LOGIN_HASH */    0,
            /* SST_IN_KEY_USER_REGION */        0,
            /* SST_IN_KEY_STRUCT_KEY */         0,
            /* SST_IN_KEY_URL */                1,
            /* SST_IN_KEY_OWNER  */             1,
            /* SST_IN_KEY_URL_MASK */           1,
            /* SST_IN_KEY_ZDOCID */             1,
            /* SST_IN_KEY_QUERY_DOPPEL_TOKEN */ 0,
            /* SST_IN_KEY_QUERY_DOPPEL_PAIR */  0,
            /* SST_IN_KEY_URL_NO_CGI */  1,
        };
        static_assert(Y_ARRAY_SIZE(splittable) == SST_IN_KEY_COUNT, "forgot to insert a new key here");
        Y_ENSURE(SubkeyTypeIsValidForKey(sst), "invalid subkey " << (int)sst);
        return splittable[sst];
    }

    bool IsSplittable(const TSaaSKeyType& kt) {
        for (auto sst : kt) {
            if (IsSplittable(sst)) {
                return true;
            }
        }
        return false;
    }

    template <class Transform>
    static TSaaSKeyType DoReorderKeyBy(const TSaaSKeyType& keyType, Transform transform) {
        TVector<std::pair<size_t, ESaaSSubkeyType>> tmp;
        tmp.reserve(keyType.size());
        for (auto kt : keyType) {
            tmp.emplace_back(transform(kt));
        }
        StableSort(tmp.begin(), tmp.end());
        TSaaSKeyType res;
        res.reserve(keyType.size());
        for (auto kt : tmp) {
            res.emplace_back(kt.second);
        }
        return res;

    }

    static auto DoGetTrieKeyOrderForSearch(ESaaSSubkeyType sst) {
        static const size_t order[] {
            /* SST_INVALID */                   Max<size_t>(),
            /* SST_IN_KEY_QUERY_STRONG */       0,
            /* SST_IN_KEY_QUERY_DOPPEL */       0,
            /* SST_IN_KEY_USER_ID */            0,
            /* SST_IN_KEY_USER_LOGIN_HASH */    0,
            /* SST_IN_KEY_USER_REGION */        3,
            /* SST_IN_KEY_STRUCT_KEY */         2,
            /* SST_IN_KEY_URL */                1,
            /* SST_IN_KEY_OWNER  */             1,
            /* SST_IN_KEY_URL_MASK */           1,
            /* SST_IN_KEY_ZDOCID */             1,
            /* SST_IN_KEY_QUERY_DOPPEL_TOKEN */ 0,
            /* SST_IN_KEY_QUERY_DOPPEL_PAIR */  0,
            /* SST_IN_KEY_URL_NO_CGI */  1,
        };
        static_assert(Y_ARRAY_SIZE(order) == SST_IN_KEY_COUNT, "forgot to insert a new key here");
        Y_ENSURE(SubkeyTypeIsValidForKey(sst), "invalid subkey " << (int)sst);
        return std::make_pair(order[sst], sst);
    }

    const TVector<ESaaSSubkeyType>& GetAllTrieSubkeysForSearch() {
        static const TVector<ESaaSSubkeyType> keys = ReorderTrieKeyForSearch(GetAllInKeySubkeys());
        return keys;
    }

    TSaaSKeyType ReorderTrieKeyForSearch(const TSaaSKeyType& keyType) {
        return DoReorderKeyBy(keyType, DoGetTrieKeyOrderForSearch);
    }

    bool SubkeyTypeIsValidForKey(ESaaSSubkeyType sst) {
        return sst > SST_INVALID && sst < SST_IN_KEY_COUNT;
    }

    bool SubkeyTypeIsValidForValue(ESaaSSubkeyType sst) {
        return sst > SST_IN_VALUE && sst < SST_IN_VALUE_COUNT;
    }

    bool SubkeyTypeIsValid(ESaaSSubkeyType sst) {
        return SubkeyTypeIsValidForKey(sst) || SubkeyTypeIsValidForValue(sst);
    }

    bool SubkeyTypeAllowedInKeyAlone(ESaaSSubkeyType sst) {
        return SubkeyTypeIsValidForKey(sst) && SST_IN_KEY_USER_REGION != sst;
    }

    bool KeyTypeIsValid(const TSaaSKeyType& key) {
        if (key.empty()) {
            return false;
        }

        bool seenType[SST_IN_KEY_COUNT];
        Zero(seenType);
        for (auto sst : key) {
            if (!SubkeyTypeIsValidForKey(sst) || seenType[sst]) {
                return false;
            }
            seenType[sst] = true;
        }
        return true;
    }

    void ParseFromString(TSaaSKeyType& kt, TStringBuf s) {
        kt.clear();
        for (const auto& tok : StringSplitter(s).Split('-').SkipEmpty()) {
            kt.emplace_back(FromString(tok.Token()));
        }
        Y_ENSURE(KeyTypeIsValid(kt), "invalid key type " << s);
    }

    void WriteToString(TString& str, const TSaaSKeyType& vec) {
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (it != vec.begin()) {
                str.append('-');
            }
            str.append(ToString(*it));
        }
    }
}

template <>
NQueryDataSaaS::TSaaSKeyType FromStringImpl<NQueryDataSaaS::TSaaSKeyType, char>(const char* data, size_t len) {
    using namespace NQueryDataSaaS;
    TSaaSKeyType vec;
    ParseFromString(vec, {data, len});
    return vec;
}

template <>
void Out<NQueryDataSaaS::TSaaSKeyType>(IOutputStream& o, const NQueryDataSaaS::TSaaSKeyType& vec) {
    TString s;
    NQueryDataSaaS::WriteToString(s, vec);
    o.Write(s);
}
