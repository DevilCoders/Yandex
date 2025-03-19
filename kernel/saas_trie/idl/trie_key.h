#pragma once

#include <kernel/qtree/compressor/factory.h>

#include <kernel/saas_trie/idl/saas_trie.pb.h>
#include <bitset>

namespace NSaasTrie {
    enum class ETrieKeyType {
        Text /* "text" */,
        ComplexKey /* "complex_key" */,
        ComplexKeyPacked /* "complex_key_packed" */
    };

    constexpr size_t RealmPrefixSetSize = sizeof(decltype(TComplexKey().GetRealmPrefixSet()));
    constexpr size_t MaxNumberOfPrefixRealms = RealmPrefixSetSize * 8;
    constexpr size_t MaxNumberOfRealms = MaxNumberOfPrefixRealms + 1;
    using TRealmPrefixSet = std::bitset<MaxNumberOfPrefixRealms>;

    TRealmPrefixSet ReadRealmPrefixSet(const NSaasTrie::TComplexKey& key);
    void WriteRealmPrefixSet(NSaasTrie::TComplexKey& key, TRealmPrefixSet);
    void WriteRealmPrefixSet(NSaasTrie::TComplexKey& key, std::initializer_list<int> prefixIndices);

    void DeserializeFromCgi(NSaasTrie::TComplexKey&, TStringBuf, bool packed);
    TString SerializeToCgi(const NSaasTrie::TComplexKey&, bool pack);
    TString SerializeToCgi(const NSaasTrie::TComplexKey&, TCompressorFactory::EFormat format);
}
