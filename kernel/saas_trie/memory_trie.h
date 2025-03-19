#pragma once

#include "abstract_trie.h"

#include <util/generic/ptr.h>
#include <util/generic/hash_set.h>

namespace NSaasTrie {
    TAtomicSharedPtr<ITrieStorage> CreateMemoryTrie();

    ui64 RemoveValuesFromTrie(ITrieStorage& storage, THashSet<ui64> valuesToRemove);
}
