#pragma once

#include <util/generic/fwd.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>

typedef TVector<ui32> TPositions;

class TUrl2PositionsReader {
private:
    TBlob Data;
    TTrieMap<ui64> Url2Offset;
    TYVector<ui32> Positions;
    TSingleValue<ui32> Size;

public:
    TUrl2PositionsReader(const TString& filename);
    size_t GetNUrls() const;
    bool Get(const TString& url, TPositions* result) const;
};

typedef THashMap<TString, TPositions> TUrl2Positions;
typedef TVector<TString> TUrls;
void MakeUrl2Positions(const TUrls& urls, TUrl2Positions* url2positions);
void WriteUrl2Positions(size_t nUrls, const TUrl2Positions& url2positions, const TString& filename);
