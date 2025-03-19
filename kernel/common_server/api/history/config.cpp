#include "config.h"
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <util/generic/serialized_enum.h>

bool THistoryConfig::DefaultNeedLock = true;
TDuration THistoryConfig::DefaultDeep = TDuration::Days(30);
TDuration THistoryConfig::DefaultMaxHistoryDeep = TDuration::Days(30);
TDuration THistoryConfig::DefaultEventsStoreInMemDeep = TDuration::Max();

TMaybe<TDuration> THistoryConfig::DefaultPingPeriod;

THistoryConfig& THistoryConfig::SetDeep(const TDuration d) {
    Deep = d;
    MaxHistoryDeep = Max(MaxHistoryDeep, d);
    return *this;
}

void THistoryConfig::Init(const TYandexConfig::Section* section) {
    NeedLock = section->GetDirectives().Value("NeedLock", NeedLock);
    EventsStoreInMemDeep = section->GetDirectives().Value("EventsStoreInMemDeep", EventsStoreInMemDeep);
    PingPeriod = section->GetDirectives().Value("PingPeriod", DefaultPingPeriod.GetOrElse(PingPeriod));
    GuaranteeFinishedTransactionsDuration = section->GetDirectives().Value("GuaranteeFinishedTransactionsDuration", GuaranteeFinishedTransactionsDuration);
    FirstLockWaitingDuration = section->GetDirectives().Value("FirstLockWaitingDuration", FirstLockWaitingDuration);
    MaxHistoryDeep = section->GetDirectives().Value("MaxHistoryDeep", MaxHistoryDeep);
    SetDeep(section->GetDirectives().Value("Deep", Deep));
    UseDeepTagsLoading = section->GetDirectives().Value("UseDeepTagsLoading", UseDeepTagsLoading);
    UseUndeadTagsLoading = section->GetDirectives().Value("UseUndeadTagsLoading", UseUndeadTagsLoading);
    MaximalLockedIntervalDuration = section->GetDirectives().Value("MaximalLockedIntervalDuration", MaximalLockedIntervalDuration);
    ChunkSize = section->GetDirectives().Value("ChunkSize", ChunkSize);
    AssertCorrectConfig(MaximalLockedIntervalDuration > GuaranteeFinishedTransactionsDuration, "MaximalLockedIntervalDuration > GuaranteeFinishedTransactionsDuration");
    ui64 lastInstantSeconds;
    if (section->GetDirectives().GetValue("LastInstant", lastInstantSeconds)) {
        LastInstant = TInstant::Seconds(lastInstantSeconds);
    } else {
        LastInstant = TInstant::Max();
    }
}

void THistoryConfig::ToString(IOutputStream& os) const {
    os << "NeedLock: " << NeedLock << Endl;
    os << "EventsStoreInMemDeep: " << EventsStoreInMemDeep << Endl;
    os << "PingPeriod: " << PingPeriod << Endl;
    os << "Deep: " << Deep << Endl;
    os << "GuaranteeFinishedTransactionsDuration: " << GuaranteeFinishedTransactionsDuration << Endl;
    os << "FirstLockWaitingDuration: " << FirstLockWaitingDuration << Endl;
    os << "MaxHistoryDeep: " << MaxHistoryDeep << Endl;
    os << "UseDeepTagsLoading: " << UseDeepTagsLoading << Endl;
    os << "UseUndeadTagsLoading: " << UseUndeadTagsLoading << Endl;
    os << "MaximalLockedIntervalDuration: " << MaximalLockedIntervalDuration << Endl;
    os << "ChunkSize: " << ChunkSize << Endl;
    if (LastInstant != TInstant::Max()) {
        os << "LastInstant: " << LastInstant.Seconds() << Endl;
    }

}
