#pragma once

#include "abstract_trie.h"

#include <kernel/saas_trie/idl/trie_key.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NSaasTrie {

    struct TComplexKeyPreprocessed {
        TString Prefix;
        TString UrlMaskPrefix;
        TVector<TVector<TString>> SortedRealms;
        TRealmPrefixSet RealmPrefixSet;
        bool LastDimensionUnique = false;
    };

    TComplexKeyPreprocessed PreprocessComplexKey(const TComplexKey&, bool needSort, const TString& uniqueDimension);

    THolder<ITrieStorageIterator> CreateTrieComplexKeyIterator(const ITrieStorageReader& trie,
                                                               const TComplexKeyPreprocessed&);
}
