#pragma once

#include "abstract_trie.h"

#include <util/generic/string.h>

namespace NSaasTrie {
    struct IDiskIO;

    void BuildTrieFromIterator(const TString& path, const IDiskIO& disk, ITrieStorageIterator& inputIterator);

    THolder<ITrieStorageWriter> CreateTrieBuilder(TString path, const IDiskIO& disk);
}
