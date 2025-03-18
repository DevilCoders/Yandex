#pragma once

#include "policies.h"

#include <util/generic/maybe.h>

namespace NSS {
    TLazySignerPtr SignByRsaKey(const TString& keyPath);

    std::vector<TLazySignerPtr> SignByAny(const TMaybe<TStringBuf>& username = Nothing());
}
