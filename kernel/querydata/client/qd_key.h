#pragma once

#include <kernel/querydata/common/querydata_traits.h>

#include <util/generic/hash.h>
#include <util/digest/numeric.h>
#include <util/str_stl.h>

namespace NQueryData {

    struct TSubkey {
        TStringBuf Key;
        int KeyType;

        TSubkey(TStringBuf k = TStringBuf(), int keytype = 0)
            : Key(k)
            , KeyType(keytype)
        {}

        auto AsPair() const {
            return std::make_pair(KeyType, Key);
        }

        friend bool operator==(const TSubkey& a, const TSubkey& b) {
            return a.AsPair() == b.AsPair();
        }

        friend bool operator!=(const TSubkey& a, const TSubkey& b) {
            return !(a == b);
        }

        friend bool operator<(const TSubkey& a, const TSubkey& b) {
            return a.AsPair() < b.AsPair();
        }

        size_t Hash() const {
            return CombineHashes(ComputeHash(Key), NumericHash(KeyType));
        }
    };

    using TSubkeys = TVector<TSubkey>;

    struct TKey {
        TSubkeys Subkeys;
        bool Common = false;

        explicit TKey(bool common = false)
            : Common(common)
        {}

        TKey(TStringBuf k, int s = 0)
        {
            Add(k, s);
        }

        TString Dump() const;

        void Add(TStringBuf k, int s) {
            Add(TSubkey(k, s));
        }

        void Add(const TSubkey& sk) {
            Subkeys.push_back(sk);
        }

        TStringBuf First() const {
            return Subkeys.empty() ? TStringBuf() : Subkeys.front().Key;
        }

        TStringBuf OfType(int type) const {
            for (const TSubkey& subkey : Subkeys) {
                if (subkey.KeyType == type) {
                    return subkey.Key;
                }
            }
            return TStringBuf();
        }

        TStringBuf OfType(TStringBuf type) const {
            return OfType(KeyTypeDescrByName(type).KeyType);
        }

        friend bool operator==(const TKey& a, const TKey& b) {
            return a.Subkeys == b.Subkeys && a.Common == b.Common;
        }

        friend bool operator!=(const TKey& a, const TKey& b) {
            return !(a == b);
        }

        friend bool operator<(const TKey& a, const TKey& b) {
            return a.Common > b.Common || (a.Common == b.Common && a.Subkeys < b.Subkeys);
        }

        size_t Hash() const {
            size_t res = 0;
            for (TSubkeys::const_iterator it = Subkeys.begin(); it != Subkeys.end(); ++it) {
                res = CombineHashes(it->Hash(), res);
            }
            return CombineHashes(res, (size_t)Common);
        }
    };

    using TKeys = TVector<const TKey*>;

}

template <>
struct THash<NQueryData::TKey> {
    size_t operator()(const NQueryData::TKey& v) const {
        return v.Hash();
    }
};

template <>
struct TLess<const NQueryData::TKey*> {
    bool operator()(const NQueryData::TKey* a, const NQueryData::TKey* b) const {
        return *a < *b;
    }
};
