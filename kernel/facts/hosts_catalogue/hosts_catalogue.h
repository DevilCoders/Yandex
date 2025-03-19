#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/input.h>


namespace NFacts {

class THostsCatalogue {

public:
    void Load(TStringBuf hostsQualityLabel, IInputStream& hostsList);
    TMaybe<bool> IsGoodHost(TStringBuf factHostOwner) const;

private:
    THashSet<TString> GoodHosts;
    THashSet<TString> BadHosts;
};

}  // namespace NFacts
