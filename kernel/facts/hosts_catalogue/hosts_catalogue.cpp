#include "hosts_catalogue.h"

#include <util/generic/yexception.h>


namespace NFacts {

static const size_t HOSTS_LIST_SIZE_MAX = 100000;

void THostsCatalogue::Load(TStringBuf hostsQualityLabel, IInputStream& hostsList) {
    THashSet<TString>* hosts = (hostsQualityLabel == "good") ? &GoodHosts : (hostsQualityLabel == "bad") ? &BadHosts : nullptr;
    Y_ENSURE(hosts != nullptr, "unknown hosts quality label '" + TString(hostsQualityLabel) + "'");

    for (size_t cnt = 0; cnt < HOSTS_LIST_SIZE_MAX; ++cnt) {
        TString s;
        if (hostsList.ReadLine(s) == 0) {
            break;
        }

        hosts->insert(std::move(s));
    }
}

TMaybe<bool> THostsCatalogue::IsGoodHost(TStringBuf factHostOwner) const {
    if (BadHosts.contains(factHostOwner)) {
        return false;
    } else if (GoodHosts.contains(factHostOwner)) {
        return true;
    } else {
        return Nothing();
    }
}

}  // namespace NFacts
