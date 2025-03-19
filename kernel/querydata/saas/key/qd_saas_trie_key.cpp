#include "qd_saas_trie_key.h"

#include <util/generic/yexception.h>
#include <util/string/cast.h>

namespace NQueryDataSaaS {
    const TString& SubkeyTypeForTrie(ESaaSSubkeyType sst) {
        static const TVector<TString> names = ([]() {
            TVector<TString> res;
            for (int i = SST_INVALID; i < SST_IN_KEY_COUNT; ++i) {
                res.emplace_back(ToString((ui32)i));
            }
            return res;
        })();
        Y_ENSURE(SubkeyTypeIsValidForKey(sst), "invalid subkey " << (int)sst);
        return names[sst];
    }

    ESaaSSubkeyType ParseTrieSubkeyType(TStringBuf str) {
        return (ESaaSSubkeyType)FromString<ui32>(str);
    }

    namespace {
        void DoAppendPrefix(TString& prefix, TStringBuf subprefix) {
            prefix.append('.').append(subprefix);
        }

        void DoAppendPrefix(TString& prefix, ESaaSSubkeyType sst) {
            DoAppendPrefix(prefix, SubkeyTypeForTrie(sst));
        }

        void DoAppendKey(TString& key, TStringBuf subkey) {
            key.append(TSaaSTrieKey::Delimiter).append(subkey);
        }
    }

    TString TSaaSTrieKey::AsString() const {
        return Prefix + Key;
    }

    TSaaSTrieKey& TSaaSTrieKey::Append(const TSaaSTrieKey& k) {
        DoAppendPrefix(Prefix, k.Prefix);
        DoAppendKey(Key, k.Key);
        return *this;
    }

    TSaaSTrieKey& TSaaSTrieKey::Append(ESaaSSubkeyType sst, TStringBuf sk) {
        DoAppendPrefix(Prefix, sst);
        DoAppendKey(Key, sk);
        return *this;
    }

    TSaaSTrieKey TSaaSTrieKey::From(ESaaSSubkeyType sst, TStringBuf sk) {
        return TSaaSTrieKey().Append(sst, sk);
    }

    TSaaSTrieKey TSaaSTrieKey::From(const TSaaSSubkey& sk) {
        return From(sk.GetType(), sk.GetKey());
    }

    TSaaSTrieKey TSaaSTrieKey::From(const TSaaSKey& key) {
        TSaaSTrieKey res;
        // TODO(velavokr): без типов в траях возможны коллизии. Нужно понять, насколько они безопасны.
        for (const auto& sk : key) {
            res.Append(sk.GetType(), sk.GetKey());
        }
        return res;
    }

    TSaaSTrieKey TSaaSTrieKey::Parse(TStringBuf key) {
        if (!key) {
            return TSaaSTrieKey();
        }

        TStringBuf pref, val;
        Y_ENSURE(key.TrySplitAt(key.find(TSaaSTrieKey::Delimiter), pref, val), "invalid key " << key);
        TSaaSTrieKey res;
        res.Prefix = pref;
        res.Key = val;
        return res;
    }

    void TSaaSTrieKey::GenerateMainKey(TString& mKey, const TSaaSKeyType& kt) {
        for (auto sst : kt) {
            DoAppendPrefix(mKey, sst);
        }
    }

    void TSaaSTrieKey::GenerateRealmKey(TString& rKey, TStringBuf key) {
        DoAppendKey(rKey, key);
    }
}

template <>
NQueryDataSaaS::TSaaSTrieKey FromStringImpl<NQueryDataSaaS::TSaaSTrieKey, char>(const char* data, size_t len) {
    using namespace NQueryDataSaaS;
    return TSaaSTrieKey::Parse({data, len});
}

template <>
void Out<NQueryDataSaaS::TSaaSTrieKey>(IOutputStream& o, const NQueryDataSaaS::TSaaSTrieKey& k) {
    using namespace NQueryDataSaaS;
    o.Write(k.AsString());
}
