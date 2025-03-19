#include "trie_key.h"

#include <kernel/qtree/compressor/factory.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/yexception.h>

namespace NSaasTrie {
    namespace {
        void ReplaceLastChars(char* begin, char* end, char from, char to) {
            for (char* ptr = end; ptr != begin; --ptr) {
                char* val = ptr - 1;
                if (*val == from) {
                    *val = to;
                } else {
                    break;
                }
            }
        }

        constexpr bool IsLong = RealmPrefixSetSize == sizeof(unsigned long);
        constexpr bool IsLongLong = RealmPrefixSetSize == sizeof(unsigned long long);
        static_assert(IsLong || IsLongLong, "size of RealmPrefixSet is incorrect");

        template<bool Long>
        inline void CopyRealmPrefixSet(NSaasTrie::TComplexKey& key, TRealmPrefixSet set) {
            key.SetRealmPrefixSet(set.to_ulong());
        }
        template<>
        inline void CopyRealmPrefixSet<false>(NSaasTrie::TComplexKey& key, TRealmPrefixSet set) {
            key.SetRealmPrefixSet(set.to_ullong());
        }
    }

    TRealmPrefixSet ReadRealmPrefixSet(const NSaasTrie::TComplexKey& key) {
        if (key.HasRealmPrefixSet()) {
            return TRealmPrefixSet(key.GetRealmPrefixSet());
        };
        return TRealmPrefixSet();
    }

    void WriteRealmPrefixSet(NSaasTrie::TComplexKey& key, TRealmPrefixSet set) {
        CopyRealmPrefixSet<IsLong>(key, set);
    }

    void WriteRealmPrefixSet(NSaasTrie::TComplexKey& key, std::initializer_list<int> prefixIndices) {
        TRealmPrefixSet set;
        for (auto index : prefixIndices) {
            Y_ENSURE(static_cast<size_t>(index) < set.size());
            set[index] = 1;
        }
        CopyRealmPrefixSet<IsLong>(key, set);
    }

    void DeserializeFromCgi(NSaasTrie::TComplexKey& key, TStringBuf data, bool packed) {
        TString decoded;
        if (data && data.back() == '.') {
            TString tmp;
            tmp = data;
            char* beg = tmp.Detach();
            ReplaceLastChars(beg, beg + tmp.size(), '.', ',');
            decoded = Base64Decode(tmp);
        } else {
            decoded = Base64Decode(data);
        }

        if (packed) {
            decoded = TCompressorFactory::DecompressFromString(decoded);
        }
        Y_ENSURE(key.ParseFromString(decoded), "could not parse");
    }

    TString SerializeToCgi(const NSaasTrie::TComplexKey& key, bool pack) {
        TString encoded = key.SerializeAsString();
        if (pack) {
            encoded = TCompressorFactory::CompressToString(encoded, TCompressorFactory::LZ_LZ4);
        }
        encoded = Base64EncodeUrl(encoded);

        if (encoded && encoded.back() == ',') {
            char* beg = encoded.Detach();
            ReplaceLastChars(beg, beg + encoded.size(), ',', '.');
        }
        return encoded;
    }

    TString SerializeToCgi(const NSaasTrie::TComplexKey& key, TCompressorFactory::EFormat format) {
        TString encoded = key.SerializeAsString();
        encoded = TCompressorFactory::CompressToString(encoded, format);
        encoded = Base64EncodeUrl(encoded);

        if (encoded && encoded.back() == ',') {
            char* beg = encoded.Detach();
            ReplaceLastChars(beg, beg + encoded.size(), ',', '.');
        }
        return encoded;
    }
}

