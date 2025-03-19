#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>

class TSynsetDictReader {
private:
    TBlob Data;
    TBlob TrieData;
    TTrieMapG<ui64, false>::T Trie;
    TBlob SynsetData;
    TStringsVectorG<false>::T Synset;

public:
    typedef TVector<TString> TSynset;

    TSynsetDictReader(const TBlob& data);
    TSynsetDictReader(const TString& filename);
    void Fill(const TString& lemma, TSynset* result) const;
};
