#pragma once
#include <library/cpp/containers/comptrie/comptrie_trie.h>

#include <util/generic/hash.h>


namespace NSnippets {
    using W2WTrie = TCompactTrie<wchar16, TUtf16String>;
    extern const THashMap<TString, W2WTrie> DOMAIN_SUBSTITUTE_TRIES;
};
