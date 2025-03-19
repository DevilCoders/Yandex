#pragma once

#include <kernel/saas_trie/abstract_trie.h>

namespace NSaasTrie {
    namespace NTesting {
        void ExpectedToHave(const ITrieStorageReader& storage, TStringBuf key, ui64 expectedValue);
        void ExpectedNotToHave(const ITrieStorageReader& storage, TStringBuf key);
        void CheckIterator(THolder<ITrieStorageIterator> iterator, const TVector<std::pair<TString, ui64>>& expected);
        void CheckSubtree(THolder<ITrieStorageReader> trie, const TVector<std::pair<TString, ui64>>& expected);
    }
}
