#include "address.h"

#include <library/cpp/logger/global/global.h>

#include <library/cpp/dns/cache.h>

TMaybe<const NDns::TResolvedHost*> NUtil::Resolve(const TString& host, ui16 port) noexcept {
    try {
        NDns::TResolveInfo info(host, port);
        const NDns::TResolvedHost* resolved = NDns::CachedResolve(info);
        if (resolved) {
            return resolved;
        }
    } catch (const TNetworkResolutionError& e) {
        ERROR_LOG << e.what() << Endl;
    }

    return Nothing();
}
