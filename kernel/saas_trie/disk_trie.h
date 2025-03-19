#pragma once

#include "abstract_trie.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NSaasTrie {
    struct IDiskIO;

    TAtomicSharedPtr<ITrieStorage> OpenDiskTrie(const TString& path, const IDiskIO& disk, bool lockMemory = false);
    bool CheckDiskTrieIsValid(const TString& path, const IDiskIO& disk);
}
