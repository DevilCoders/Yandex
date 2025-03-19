#pragma once

#include "abstract_trie.h"

#include <util/generic/ptr.h>

namespace NSaasTrie {
    THolder<ITrieStorageIterator> FilterDuplicateKeys(THolder<ITrieStorageIterator> source);
}

