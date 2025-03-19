#pragma once

#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>
#include <kernel/mirrors/url_filter.h>
#include "canonizers.h"

namespace NRealTime {
    class TDocFactors;
}

class TCatalogData : public TCanonizers {
public:
    typedef TTrieSetG<false>::T THostSet;

    THostSet ManualAdultList;
    THostSet ManualNonAdultList;
    THostSet PornoMenuOwnersList;

    TCatalogData(const TBlob& blob);
};

class TCatalogReader {
    THolder<TMemoryMap> Mf;
    THolder<TCatalogData> CatalogData;

    THolder<TBadUrlFilter> ManualAdultList;
    THolder<TBadUrlFilter> ManualNonAdultList;
    THolder<TBadUrlFilter> PornoMenuOwnersList;
public:
    TCatalogReader(const TString& filename);
    void FillAdultFactors(const char* pszUrl, NRealTime::TDocFactors& proto) const;
};
