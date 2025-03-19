#pragma once

#include "catfilter_wrapper.h"

#include <kernel/catfilter/catfiltertrie.pb.h>
#include <kernel/mirrors/mirrors_trie.h>

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/generic/vector.h>
#include <util/string/vector.h>

#define MAXMILLIONS 256

class TCatFiller;

class TCatPacker {
public:
    void UnpackLeaf(const char* p, TCatEntry& c) const;
    void PackLeaf(char* p, const TCatEntry& entry, size_t size) const;
    size_t MeasureLeaf(const TCatEntry& entry) const;
    size_t SkipLeaf(const char* p) const;
};

class TCatFilterTrie : public ICatFilter {
public:
    enum MapMode {
        mDefault,
        mLocked,
    };

    typedef TCompactTrie<char, TCatEntry, TCatPacker> TTrie;

    TCatAttrsPtr DoFind(const TString& url) const;
    TCatAttrsPtr Find(const TString& url) const noexcept override;

    void Print() override;

    // Valid values for additional_mode are oPrecharge and oLocked.
    TCatFilterTrie(const TString& filename, TMirrorsMappedTrie* mirrors = nullptr,
                   MapMode mode = mDefault);
    TCatFilterTrie(char* data, size_t len, TMirrorsMappedTrie* mirrors);

    ~TCatFilterTrie() override;

    TTrie::TConstIterator Begin() const;
    TTrie::TConstIterator End() const;
    bool Next(TTrie::TConstIterator& it) const;

private:
    MapMode Mode;
    THolder<TMappedFile> MappedFile;
    THolder<TTrie> Trie;
    TMirrorsMappedTrie* Mirrors;

    void PrintEntry(const TCatEntry& entry);

    void CollectQuery(const TTrie& trie, TVector<TString>::iterator begin, TVector<TString>::iterator end, TCatFiller&) const;
    void CollectPath(TVector<TString>::iterator begin, TVector<TString>::iterator end, TCatFiller&) const;
};

TVector<TString>* UrlToComponents(const TString& str, bool addRootDir = false, TMirrorsMappedTrie* mirrors = nullptr);
TString ExtractQuery(TVector<TString>& components);
