#pragma once

#include <kernel/common_server/util/accessor.h>
#include <util/generic/maybe.h>
#include <util/datetime/base.h>
#include <util/system/mutex.h>


template <class TKey, class TData>
class TCacheWithLiveTime {
private:

    class TTSKey {
    private:
        RTLINE_ACCEPTOR_DEF(TTSKey, Key, TKey);
        RTLINE_ACCEPTOR_DEF(TTSKey, TS, TInstant);
    public:
        TTSKey(const TKey& key, const TInstant ts)
            : Key(key)
            , TS(ts)
        {
        }
    };

    class TTSData {
    private:
        RTLINE_ACCEPTOR_DEF(TTSData, Data, TData);
        RTLINE_ACCEPTOR_DEF(TTSData, TS, TInstant);
    public:
        TTSData() = default;
        TTSData(const TTSData&) = default;
        TTSData(TData&& data, const TInstant ts)
            : Data(std::move(data))
            , TS(ts)
        {
        }
    };

    TMutex Mutex;
    TMap<TString, TTSData> Cache;
    TDeque<TTSKey> TSChecker;
    TDuration LiveTime = TDuration::Minutes(1);
public:

    TCacheWithLiveTime(const TDuration liveTime)
        : LiveTime(liveTime)
    {

    }

    void EraseExpired(const TInstant now = Now()) {
        TGuard<TMutex> g(Mutex);
        EraseExpiredUnsafe(now);
    }

    TMaybe<TData> GetValue(const TKey& key) {
        TGuard<TMutex> g(Mutex);
        auto it = Cache.find(key);
        if (it == Cache.end()) {
            return TMaybe<TData>();
        } else {
            return it->second.GetData();
        }
    }

    void PutValue(const TKey& key, TData&& data) {
        const TInstant current = Now();

        TGuard<TMutex> g(Mutex);
        EraseExpiredUnsafe(current);

        Cache[key] = TTSData(std::move(data), current);
        TSChecker.emplace_back(key, current);
    }
private:

    void EraseExpiredUnsafe(const TInstant now) {
        while (TSChecker.size() && (now - TSChecker.front().GetTS()) > LiveTime) {
            auto it = Cache.find(TSChecker.front().GetKey());
            if (it != Cache.end() && it->second.GetTS() == TSChecker.front().GetTS()) {
                Cache.erase(it);
            }
            TSChecker.pop_front();
        }
    }
};
