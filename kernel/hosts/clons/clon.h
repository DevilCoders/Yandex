#pragma once

#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/pool.h>

class TClonAttrCanonizer {
public:
    TClonAttrCanonizer();
    TClonAttrCanonizer(const TString& clonh2g);
    TClonAttrCanonizer(const TString& clonh2gTrieIndex, const TString& clonh2gTrieData);

    TVector<ui32> GetHostClonGroups(const char* host) const {
        TVector<ui32> groups;
        GetHostClonGroups(host, groups);
        return groups;
    }

    void GetHostClonGroups(TStringBuf host, TVector<ui32>& groups) const;

    void GetHostClonGroups(const char* host, TVector<ui32>& groups) const {
        GetHostClonGroups(TStringBuf(host), groups);
    }

    void AddClon(TStringBuf host, ui32 group, ui32 type);

    void AddClon(const char* host, ui32 group, ui32 type) {
        AddClon(TStringBuf(host), group, type);
    }

    void LoadClons(const TString& clonh2g);

private:
    struct TGroup {
        ui32 Group;
        ui32 Type;
    };
    struct TDataIndices {
        ui32 FirstIndex;
        ui32 LastIndex;
    };
    typedef TVector<TGroup> TGroupVector;
    typedef THashMap<TStringBuf, TGroupVector, TCIHash<TStringBuf>, TCIEqualTo<TStringBuf> > TClonMap;
    typedef TTrieMap<TDataIndices> TClonTrieIndex;

    TClonMap ClonMap;
    THolder<const TClonTrieIndex> ClonTrieIndex;
    TBlob ClonTrieData;

    THolder<TMemoryPool> Pool;
};
