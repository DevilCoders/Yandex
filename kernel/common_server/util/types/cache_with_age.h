#pragma once

#include <kernel/common_server/util/algorithm/iterator.h>
#include <kernel/common_server/util/accessor.h>
#include <util/generic/map.h>
#include <util/system/mutex.h>
#include <util/datetime/base.h>
#include <util/system/rwlock.h>

template <class TKey, class TDataImpl>
class TCacheWithAge {
private:
    class TData {
        RTLINE_ACCEPTOR(TData, IsExists, bool, true);
    private:
        TInstant RefreshInstant = Now();
        TDataImpl Data;

    public:
        const TDataImpl& GetData() const {
            return Data;
        }

        TInstant GetFreshness() const {
            return RefreshInstant;
        }

        TData(const TDataImpl& data)
            : IsExists(true)
            , Data(data) {

        }

        TData()
            : IsExists(false)
        {
        }
    };

private:
    TRWMutex Mutex;
    TMap<TKey, TData> Cache;

public:
    enum class ECachedValue {
        HasData,
        Absent,
        NoInfo
    };

public:
    void Drop() {
        TWriteGuard wg(Mutex);
        Cache.clear();
    }

    void Drop(const TKey& key) {
        TWriteGuard g(Mutex);
        Cache.erase(key);
    }

    void SetAbsent(const TKey& key) {
        TWriteGuard g(Mutex);
        auto res = Cache.emplace(key, TData());
        if (!res.second) {
            res.first->second = TData();
        }
    }

    void Refresh(const TKey& key, const TDataImpl& data) {
        TWriteGuard g(Mutex);
        auto res = Cache.emplace(key, data);
        if (!res.second) {
            res.first->second = TData(data);
        }
    }

    ECachedValue GetData(const TKey& key, TDataImpl& result, const TDuration maxAge) const {
        return GetData(key, result, Now() - maxAge);
    }

    ECachedValue GetData(const TKey& key, TDataImpl& result, const TInstant reqActuality = TInstant::Zero()) const {
        TReadGuard rg(Mutex);
        auto it = Cache.find(key);
        if (it != Cache.end()) {
            if (it->second.GetFreshness() > reqActuality) {
                if (!it->second.GetIsExists()) {
                    return ECachedValue::Absent;
                } else {
                    result = it->second.GetData();
                    return ECachedValue::HasData;
                }
            }
        }
        return ECachedValue::NoInfo;
    }

    template <class TOnHasData, class TOnNoData>
    void ProcessIds(const TSet<TKey>& ids, TOnHasData& actionOnHasData, TOnNoData& actionOnNoData, const TInstant reqActuality) const {
        TReadGuard rg(Mutex);
        if (ids.size() > Cache.size() / 10) {
            auto itCache = Cache.begin();
            auto itId = ids.begin();
            for (; itId != ids.end(); ++itId) {
                if (Advance(itCache, Cache.end(), *itId) && itCache->second.GetFreshness() > reqActuality) {
                    if (itCache->second.GetIsExists()) {
                        actionOnHasData(*itId, itCache->second.GetData());
                    }
                } else {
                    actionOnNoData(*itId);
                }
            }
        } else {
            for (auto&& i : ids) {
                TDataImpl info;
                auto cachedStatus = GetData(i, info, reqActuality);
                if (cachedStatus == ECachedValue::HasData) {
                    actionOnHasData(i, info);
                } else if (cachedStatus == ECachedValue::NoInfo) {
                    actionOnNoData(i);
                }
            }
        }
    }
};

