#pragma once
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/abstract/database.h>
#include <util/datetime/base.h>
#include <library/cpp/yconf/conf.h>
#include <util/stream/output.h>
#include <kernel/common_server/library/storage/structured.h>

class THistoryConfig {
public:
    static bool DefaultNeedLock;
    static TDuration DefaultDeep;
    static TDuration DefaultMaxHistoryDeep;
    static TDuration DefaultEventsStoreInMemDeep;

    static TMaybe<TDuration> DefaultPingPeriod;
private:

    RTLINE_READONLY_ACCEPTOR(Deep, TDuration, DefaultDeep);
    RTLINE_ACCEPTOR(THistoryConfig, GuaranteeFinishedTransactionsDuration, TDuration, TDuration::Minutes(2));
    RTLINE_ACCEPTOR(THistoryConfig, FirstLockWaitingDuration, TDuration, TDuration::Minutes(10));
    RTLINE_ACCEPTOR(THistoryConfig, PingPeriod, TDuration, TDuration::Seconds(1));
    RTLINE_ACCEPTOR(THistoryConfig, MaxHistoryDeep, TDuration, DefaultMaxHistoryDeep);
    RTLINE_ACCEPTOR(THistoryConfig, EventsStoreInMemDeep, TDuration, DefaultEventsStoreInMemDeep);
    RTLINE_ACCEPTOR(THistoryConfig, LastInstant, TInstant, TInstant::Max());
    RTLINE_ACCEPTOR(THistoryConfig, UseDeepTagsLoading, bool, true);
    RTLINE_ACCEPTOR(THistoryConfig, UseUndeadTagsLoading, bool, true);
    RTLINE_ACCEPTOR(THistoryConfig, NeedLock, bool, DefaultNeedLock);
    RTLINE_ACCEPTOR(THistoryConfig, MaximalLockedIntervalDuration, TDuration, TDuration::Minutes(10));
    RTLINE_ACCEPTOR(THistoryConfig, ChunkSize, size_t, 1000000);
public:
    void NoLimitsHistory() {
        Deep = TDuration::Max();
        MaxHistoryDeep = TDuration::Max();
        EventsStoreInMemDeep = TDuration::Max();
    }

    THistoryConfig() {
    }

    THistoryConfig& SetDeep(const TDuration d);

    void Init(const TYandexConfig::Section* section);

    void ToString(IOutputStream& os) const;
};
