#pragma once

#include "cache_with_live_time.h"
#include <util/system/rwlock.h>


template <class TKey, class TData>
class TCachesPool {
private:
    using TCache = TCacheWithLiveTime<TKey, TData>;
    TMap<TString, TCache> Caches;
    TRWMutex RWMutex;
    TDuration LiveTime = TDuration::Minutes(1);
public:

    TCachesPool(const TDuration liveTime)
        : LiveTime(liveTime)
    {

    }

    TCache& GetCache(const TString& cacheId) {
        {
            TReadGuard rg(RWMutex);
            auto it = Caches.find(cacheId);
            if (it != Caches.end()) {
                return it->second;
            }
        }
        {
            TWriteGuard rg(RWMutex);
            return Caches.emplace(cacheId, TCache(LiveTime)).first->second;
        }
    }
};
