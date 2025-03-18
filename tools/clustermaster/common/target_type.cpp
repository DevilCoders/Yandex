#include "target_type.h"

#include "make_vector.h"

#include <library/cpp/containers/sorted_vector/sorted_vector.h>

#include <util/generic/hash_set.h>
#include <util/generic/yexception.h>

TVector<TString> NTargetTypePrivate::ExpandHosts(const TVector<TString>& hosts, const TMasterListManager* listManager) {
    TVector<TString> result;

    for (TVector<TString>::const_iterator host = hosts.begin(); host != hosts.end(); host++) {
        if (host->StartsWith('!')) {
            listManager->GetList(host->substr(1), result);
        } else {
            result.push_back(*host);
        }
    }

    return StableUnique(result);
}

