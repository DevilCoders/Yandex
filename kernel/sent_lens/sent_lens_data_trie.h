#pragma once

#include <library/cpp/containers/comptrie/comptrie_trie.h>

// Trie contains all the sequences that are stored in sent_lens_data_hashes2.inc file
class TSentLensDataCollectionTrie {
public:
    TSentLensDataCollectionTrie();

    // searches for the longest prefix that contains in sentlens data collection
    // returns index of the found prefix
    // actual string can be retrieved with TSentenceLengthsCoderData::GetBlockVersion2
    size_t FindLongestPrefix(const ui8* begin, size_t size, ui16* index);

private:
    TCompactTrie<ui8, ui16> Trie;
};
