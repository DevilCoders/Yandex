#pragma once

#include "log.h"
#include "sockaddr.h"

#include <util/generic/hash.h>
#include <util/system/rwlock.h>

#include <utility>

class TNetworkAddressResolver {
private:
    using TPair = std::pair<TString, TIpPort>;
    using TCache = THashMap<TPair, TNetworkAddress>;

public:
    const TNetworkAddress& operator() (const TString& host, TIpPort port) {
        TPair pair = std::make_pair(host, port);
        {
            DEBUGLOG("Resolve address " << host << ":" << port);
            TReadGuard rguard(Mutex);
            const auto it = Cache.find(pair);
            if (it != Cache.end()) {
                return it->second;
            }
        }
        DEBUGLOG("Not found in cache. Resolving...");
        TWriteGuard wguard(Mutex);
        TNetworkAddress addr = TSocketAddress(host, port).GetNetworkAddress();
        auto res = Cache.insert(TCache::value_type(pair, addr));
        return res.first->second;
    }

private:
    TCache Cache;
    TRWMutex Mutex;
};
