#pragma once

#include "common.h"
#include "config.h"
#include "filter.h"

#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/settings/abstract/abstract.h>
#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/api/common.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/util/instant_model.h>

#include <kernel/daemon/common/time_guard.h>
#include <kernel/daemon/messages.h>

#include <library/cpp/mediator/messenger.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/future/async.h>

#include <util/digest/fnv.h>
#include <util/generic/set.h>
#include <util/generic/adaptor.h>
#include <util/string/join.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/rusage.h>
#include <util/system/rwlock.h>
#include <util/thread/pool.h>
#include "db_owner.h"

class TConfigurableCheckers {
    RTLINE_ACCEPTOR(TConfigurableCheckers, HistoryParsingAlertsActive, bool, false);
public:
    bool AlertHistoryParsingOnFail(const bool isCacheBuilding) const {
        return isCacheBuilding || !HistoryParsingAlertsActive;
    }
};

class TCommonCacheInvalidateNotification: public NMessenger::IMessage {
    RTLINE_ACCEPTOR_DEF(TCommonCacheInvalidateNotification, TableName, TString);
    RTLINE_ACCEPTOR(TCommonCacheInvalidateNotification, ReqActuality, TInstant, TInstant::Zero());
    RTLINE_ACCEPTOR_DEF(TCommonCacheInvalidateNotification, OriginatorTableName, TString);
public:
    TCommonCacheInvalidateNotification() = default;
    TCommonCacheInvalidateNotification(const TString& tableName, const TInstant reqActuality, const TString& originatorTableName)
        : TableName(tableName)
        , ReqActuality(reqActuality)
        , OriginatorTableName(originatorTableName) {

    }
};

class TAfterCacheInvalidateNotification: public TCommonCacheInvalidateNotification {
public:
    using TCommonCacheInvalidateNotification::TCommonCacheInvalidateNotification;
};

class TBeforeCacheInvalidateNotification: public TCommonCacheInvalidateNotification {
public:
    using TCommonCacheInvalidateNotification::TCommonCacheInvalidateNotification;
};

class TNotifyHistoryChanged: public NMessenger::IMessage {
    RTLINE_ACCEPTOR_DEF(TNotifyHistoryChanged, TableName, TString);
    RTLINE_ACCEPTOR(TNotifyHistoryChanged, RecordsCount, ui32, 0);
    RTLINE_ACCEPTOR(TNotifyHistoryChanged, ReqActuality, TInstant, TInstant::Zero());
public:
    TNotifyHistoryChanged() = default;
    TNotifyHistoryChanged(const TString& tableName, const ui32 recordsCount, const TInstant reqActuality)
        : TableName(tableName)
        , RecordsCount(recordsCount)
        , ReqActuality(reqActuality) {

    }
};


class IBaseSequentialTableImpl: public NCS::TDatabaseOwner {
private:
    using TBase = NCS::TDatabaseOwner;
    const TString TableName;
protected:
    virtual TString GetTimestampFieldName() const = 0;
    virtual TString GetSeqFieldName() const = 0;

    bool GetNextInterval(const i64 min, const i64 max, const TInstant instant, i64& minNext, i64& maxNext, NStorage::ITransaction::TPtr transaction) const;
    TInstant GetInstantByHistoryEventId(const i64 historyEventId, NStorage::ITransaction::TPtr transaction) const;
    i64 GetSuppEventIdNearest(const i64 historyEventId, const i64 min, NStorage::ITransaction::TPtr transaction) const;
    i64 GetHistoryEventIdByTimestamp(const TInstant instant, const TString& info) const;

public:
    virtual ~IBaseSequentialTableImpl() = default;

    TString GetTableName() const {
        return TableName;
    }

    IBaseSequentialTableImpl(NStorage::IDatabase::TPtr database, const TString& tableName)
        : TBase(database)
        , TableName(tableName)
    {

    }
};


class THistoryConditionsTimeline {
private:
    TMap<TDuration, TMap<TString, TSet<TString>>> Conditions;
    TDuration DefaultDeep = TDuration::Days(45);
    bool EnableDeep = true;
    bool EnableUndead = true;
public:
    THistoryConditionsTimeline(const TDuration maxDeep, bool useDeep, bool useUndead)
        : DefaultDeep(maxDeep)
        , EnableDeep(useDeep)
        , EnableUndead(useUndead)
    {}

    void AddUndeadObject(const TString& name, const TString& value) {
        if (EnableUndead) {
            AddObject(name, value, TDuration::Max());
        }
    }

    void AddObject(const TString& name, const TString& value) {
        AddObject(name, value, DefaultDeep);
    }

    void AddObject(const TString& name, const TString& value, TDuration historyDeep) {
        if (EnableDeep) {
            Conditions[historyDeep][name].emplace(value);
        }
    }

    template <class TContainer>
    void AddObject(const TString& name, const TContainer& data) {
        AddObject(name, data, DefaultDeep);
    }

    template <class TContainer>
    void AddObject(const TString& name, const TContainer& data, TDuration historyDeep) {
        if (EnableDeep) {
            Conditions[historyDeep][name].insert(data.begin(), data.end());
        }
    }

    TMap<TDuration, TString> BuildFinalConditions();
};

template <class TEvent>
class TBaseSequentialTableImpl: public IBaseSequentialTableImpl, public IMessageProcessor, public IAutoActualization {
    using TSelf = TBaseSequentialTableImpl<TEvent>;

public:
    using THistoryEvent = TEvent;
    using TEventPtr = TAtomicSharedPtr<TEvent>;
    using TEvents = TVector<TEventPtr>;

protected:
    using TEventId = typename TEvent::TEventId;
    using TUserId = TStringBuf;
    mutable TRWMutex Mutex;
    mutable TEventId LockedMaxEventId = TEvent::IncorrectEventId;
    mutable TInstant LastSuccessReadInstant = TInstant::Zero();

    mutable TInstant StartInstant = TInstant::Zero();
private:
    NStorage::IDatabase::TPtr DB;
    mutable TInstant CurrentInstant = TInstant::Zero();
    mutable TInstant LastSuccessLockInstant = TInstant::Zero();
    mutable TEventId CurrentMaxEventId = TEvent::IncorrectEventId;

protected:
    mutable TDeque<TEventPtr> Events;
    mutable TAtomic InsertedEventsCount = 0;

protected:
    const THistoryConfig Config;
    THolder<THistoryContext> OwnerContext;
    const IHistoryContext& Context;
    TRWMutex MutexRequest;
    mutable TSet<ui64> ReadyEvents;
    mutable TAtomic UpdatesCounter = 0;

protected:
    virtual TString GetTimestampFieldName() const override {
        return THistoryObjectDescription<TEvent>::GetTimestampFieldName();
    }

    virtual TString GetSeqFieldName() const override {
        return THistoryObjectDescription<TEvent>::GetSeqFieldName();
    }

    virtual void CleanFinishedEvents() const {

    }

    virtual void StartCacheConstruction() const {

    }

    virtual void FinishCacheConstruction() const {
        TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("FinishCacheConstruction for " + GetTableName() + "/" + ::ToString(Events.size()) + ")");
        INFO_LOG << "RESOURCE: FinishCacheConstruction start for " << GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
        AtomicSet(InsertedEventsCount, Events.size());
        i64 eventIdPred = -1;
        for (auto&& i : Events) {
            CHECK_WITH_LOG(eventIdPred < (i64)i->GetHistoryEventId()) << eventIdPred << " / " << i->GetHistoryEventId() << Endl;
            eventIdPred = i->GetHistoryEventId();
        }

        if (Events.size()) {
            LockedMaxEventId = Events.back()->GetHistoryEventId();
            CurrentMaxEventId = Events.back()->GetHistoryEventId();
        }
        INFO_LOG << "RESOURCE: FinishCacheConstruction finish for " << GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
        CleanFinishedEvents();
    }

public:
    TInstant GetLastSuccessLockInstant() const {
        return LastSuccessLockInstant;
    }

    template <class T, class TV>
    static void InsertSorted(TV& v, T object) {
        v.emplace_back(object);
        auto itBack = v.rbegin();
        auto itBackPred = v.rbegin();
        ++itBackPred;
        const auto evId = object->GetHistoryEventId();
        for (; itBackPred != v.rend(); ++itBackPred, ++itBack) {
            if ((*itBackPred)->GetHistoryEventId() > evId) {
                std::swap(*itBackPred, *itBack);
            } else {
                CHECK_WITH_LOG((*itBackPred)->GetHistoryEventId() < object->GetHistoryEventId());
                break;
            }
        }
    }

    template <class T>
    static TVector<T> PopFrontEvent(TDeque<T>& v, const ui64 historyEventId) {
        TVector<T> result;
        while (v.size() && v.front()->GetHistoryEventId() <= historyEventId) {
            result.emplace_back(v.front());
            v.pop_front();
        }
        return result;
    }
protected:

    virtual bool GetCallbackNeedsHistoryLoad() const {
        return false;
    }

    virtual TVector<TAtomicSharedPtr<TEvent>> CleanExpiredEvents(const TInstant /*deadline*/) const {
        return TVector<TAtomicSharedPtr<TEvent>>();
    }

    virtual void CleanCurrentBuffer(const bool /*locked*/) const {
        if (LockedMaxEventId != TEvent::IncorrectEventId) {
            if (ReadyEvents.empty() || *ReadyEvents.rbegin() <= LockedMaxEventId) {
                ReadyEvents.clear();
            } else {
                ReadyEvents.erase(ReadyEvents.begin(), ReadyEvents.upper_bound(LockedMaxEventId));
            }
        }
    }

    virtual bool DoAddEvent(TAtomicSharedPtr<TEvent>& /*eventObj*/, const bool /*locked*/) const {
        return true;
    }

    bool AddEvent(TEvent&& eventObj, TInstant& actualInstant, TInstant& modelingInstantNow, const bool locked) const {
        CHECK_WITH_LOG(LockedMaxEventId == TEvent::IncorrectEventId || LockedMaxEventId < eventObj.GetHistoryEventId()) << LockedMaxEventId << " / " << eventObj.GetHistoryEventId() << Endl;
        if (ReadyEvents.contains(eventObj.GetHistoryEventId())) {
            if (LockedMaxEventId < eventObj.GetHistoryEventId()) {
                if (locked || eventObj.GetHistoryInstant() + Config.GetGuaranteeFinishedTransactionsDuration() < ModelingNow()) {
                    LockedMaxEventId = eventObj.GetHistoryEventId();
                }
            }
            return false;
        }
        ReadyEvents.emplace(eventObj.GetHistoryEventId());

       auto currentEvent = MakeAtomicShared<TEvent>(std::move(eventObj));
        InsertSorted(Events, currentEvent);
        AtomicIncrement(InsertedEventsCount);

        DoAddEvent(currentEvent, locked);

        if (modelingInstantNow < currentEvent->GetHistoryInstant()) {
            modelingInstantNow = currentEvent->GetHistoryInstant();
        }
        if (actualInstant < currentEvent->GetHistoryInstant()) {
            actualInstant = currentEvent->GetHistoryInstant();
        }
        if (locked) {
            LockedMaxEventId = currentEvent->GetHistoryEventId();
        }
        if (CurrentMaxEventId == TEvent::IncorrectEventId || CurrentMaxEventId < eventObj.GetHistoryEventId()) {
            CurrentMaxEventId = currentEvent->GetHistoryEventId();
        }
        return true;
    }

protected:
    class TParsedObjects: public NStorage::TPackedRecordsSet {
        RTLINE_READONLY_ACCEPTOR(ActualInstant, TInstant, TInstant::Zero());
        RTLINE_READONLY_ACCEPTOR(ModelingInstantNow, TInstant, TInstant::Zero());
        RTLINE_ACCEPTOR(TParsedObjects, Locked, bool, true);
        RTLINE_READONLY_ACCEPTOR(IgnoreParseErrors, bool, false);

    private:
        using TDecoder = typename TEvent::TDecoder;

    private:
        const TSelf& Owner;
        THolder<NCS::TEntitySession> Session;
        TVector<TVector<TStringBuf>> RecordsPool;
        TVector<TEvent> ParsedRecordsPool;
        TVector<ui32> ParsedRecordsPoolCorrect;
        ui32 IterationIdx = 1;
        const ui32 MTParsingPoolSize = 200000;
        THolder<TThreadPool> Queue;
        i32 CurrentIdx = -1;
        i32 CurrentIdxGlobal = 0;
        TDecoder Decoder;
        const bool IsCacheBuilding = false;
        NRTProc::TAbstractLock::TPtr Lock;

        bool IsLockInitialized() const {
            return !!Lock && Lock->IsLocked();
        }

    public:

        const TDecoder& GetDecoder() const {
            return Decoder;
        }

        i32 GetCurrentIdxGlobal() const {
            return CurrentIdxGlobal;
        }

        NCS::TEntitySession& GetSession() const {
            return *Session;
        }

        TParsedObjects(const TSelf& owner, const TInstant actualInstant, const TInstant modelingInstantNow, const bool readOnly, const bool lockRequest = false, const bool ignoreParseErrors = false)
            : ActualInstant(actualInstant)
            , ModelingInstantNow(modelingInstantNow)
            , IgnoreParseErrors(ignoreParseErrors)
            , Owner(owner)
            , IsCacheBuilding(actualInstant == TInstant::Zero())
        {
            if (lockRequest) {
                const TInstant start = Now();
                TTimeGuardImpl<false, TLOG_NOTICE> timeGuardLock("BuildCacheLock");
                while (Now() < start + Owner.GetConfig().GetFirstLockWaitingDuration()) {
                    for (i32 i = 0; i < Max<i32>(NFrontend::NSettings::GetValueDef<i32>("max_in_fly_cache_load", 1), 1); ++i) {
                        Lock = Owner.DB->Lock("BuildCacheLock-" + Owner.GetTableName() + "-" + ToString(i), true, TDuration::Zero());
                        if (IsLockInitialized()) {
                            break;
                        }
                    }
                    if (IsLockInitialized()) {
                        break;
                    }
                    Sleep(TDuration::Seconds(3));
                }
                CHECK_WITH_LOG(IsLockInitialized());
            }
            Session = Owner.BuildNativeSessionPtr(readOnly);
            Session->GetTransaction()->SetExpectFail(true);
            ParsedRecordsPoolCorrect.resize(MTParsingPoolSize * 1.5, 0);
            RecordsPool.resize(MTParsingPoolSize);
        }

    public:
        class TParseActor: public IObjectInQueue {
        private:
            const TVector<TVector<TStringBuf>>& Records;
            const ui32 CurrentCount;
            TParsedObjects& ObjectsOwner;
            TVector<TEvent>& ParsedRecordsPool;
            TVector<ui32>& ParsedRecordsPoolCorrect;
            TAtomic& ReadIndex;
            TReadGuard Guard;
            const ui32 IteratorIdx;
            const bool IsCacheBuilding = false;
            const bool IgnoreParseErrors = true;
        public:
            TParseActor(
                const TVector<TVector<TStringBuf>>& records,
                const ui32 currentCount,
                TParsedObjects& objectsOwner,
                TVector<TEvent>& parsedRecordsPool,
                TVector<ui32>& parsedRecordsPoolCorrect,
                TAtomic& readIndex,
                TRWMutex& mutex,
                const ui32 iteratorIdx,
                const bool isCacheBuilding,
                const bool ignoreParseErrors
            )
                : Records(records)
                , CurrentCount(currentCount)
                , ObjectsOwner(objectsOwner)
                , ParsedRecordsPool(parsedRecordsPool)
                , ParsedRecordsPoolCorrect(parsedRecordsPoolCorrect)
                , ReadIndex(readIndex)
                , Guard(mutex)
                , IteratorIdx(iteratorIdx)
                , IsCacheBuilding(isCacheBuilding)
                , IgnoreParseErrors(ignoreParseErrors)
            {
                CHECK_WITH_LOG(CurrentCount <= Records.size()) << CurrentCount;
            }

            virtual void Process(void* /*threadSpecificResource*/) override {
                while (true) {
                    const ui64 currentIdx = AtomicIncrement(ReadIndex) - 1;
                    if (currentIdx >= CurrentCount) {
                        break;
                    }
                    TEvent& eventObj = ParsedRecordsPool[currentIdx];
                    TConstArrayRef<TStringBuf> arrLocal(Records[currentIdx]);
                    if (!eventObj.DeserializeWithDecoder(ObjectsOwner.Decoder, arrLocal)) {
                        if (ObjectsOwner.Decoder.NeedVerboseParsingErrorLogging()) {
                            ERROR_LOG << "cannot parse from record: " + JoinSeq(", ", Records[currentIdx]) << ", table: " << ObjectsOwner.Owner.GetTableName() << Endl;
                        } else {
                            ERROR_LOG << "cannot parse from record: (...), table: " << ObjectsOwner.Owner.GetTableName() << Endl;
                        }
                        if (!IsCacheBuilding) {
                            ALERT_NOTIFY << "cannot parse from record: " + JoinSeq(", ", Records[currentIdx]) << ", table: " << ObjectsOwner.Owner.GetTableName() << Endl;
                        }
                        Y_ASSERT(Singleton<TConfigurableCheckers>()->AlertHistoryParsingOnFail(IgnoreParseErrors));
                        continue;
                    }
                    ParsedRecordsPoolCorrect[currentIdx] = IteratorIdx;
                }
            }
        };

        void ParseMT(const ui32 currentCount) {
            TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("parsing " + ToString(currentCount) + " records (to " + ToString(currentCount + (IterationIdx - 1) * MTParsingPoolSize) + " for " + Owner.GetTableName() + ")");
            if (IterationIdx == 1) {
                Queue = MakeHolder<TThreadPool>();
                Queue->Start(8);
            }
            ++IterationIdx;
            ParsedRecordsPool.clear();
            ParsedRecordsPool.resize(currentCount);
            TAtomic counter = 0;
            {
                TRWMutex mutex;
                for (ui32 i = 0; i < 8; ++i) {
                    Queue->SafeAddAndOwn(MakeHolder<TParseActor>(RecordsPool, currentCount, *this, ParsedRecordsPool, ParsedRecordsPoolCorrect, counter, mutex, IterationIdx, IsCacheBuilding, IgnoreParseErrors));
                }

                TWriteGuard wg(mutex);
            }
            if (currentCount) {
                TWriteGuard wgOwner(Owner.Mutex);
                for (ui32 i = 0; i < currentCount; ++i) {
                    if (ParsedRecordsPoolCorrect[i] == IterationIdx) {
                        if (!IsCacheBuilding) {
                            Owner.AddEvent(std::move(ParsedRecordsPool[i]), ActualInstant, ModelingInstantNow, Locked);
                        } else {
                            Owner.Events.emplace_back(MakeAtomicShared<TEvent>(std::move(ParsedRecordsPool[i])));
                        }
                    }
                }
            }
            if (!IsCacheBuilding) {
                Owner.CacheSignal()("code", "rows_parsed")(currentCount);
            }
            CurrentIdx = -1;
        }

        void ParseST(const ui32 currentCount) {
            if (currentCount) {
                TWriteGuard wgOwner(Owner.Mutex);
                for (ui32 i = 0; i < currentCount; ++i) {
                    TEvent eventObj;
                    if (!eventObj.DeserializeWithDecoder(Decoder, RecordsPool[i])) {
                        if (Decoder.NeedVerboseParsingErrorLogging()) {
                            ERROR_LOG << "cannot parse from record: " + JoinSeq(", ", RecordsPool[i]) << ", table: " << Owner.GetTableName() << Endl;
                        } else {
                            ERROR_LOG << "cannot parse from record: (...), table: " << Owner.GetTableName() << Endl;
                        }
                        if (!IsCacheBuilding) {
                            ALERT_NOTIFY << "cannot parse from record: " + JoinSeq(", ", RecordsPool[i]) << ", table: " << Owner.GetTableName() << Endl;
                        }
                        Y_ASSERT(Singleton<TConfigurableCheckers>()->AlertHistoryParsingOnFail(IsCacheBuilding));
                        continue;
                    }
                    if (!IsCacheBuilding) {
                        Owner.AddEvent(std::move(eventObj), ActualInstant, ModelingInstantNow, Locked);
                    } else {
                        Owner.Events.emplace_back(MakeAtomicShared<TEvent>(std::move(eventObj)));
                    }
                }
            }
            if (!IsCacheBuilding) {
                Owner.CacheSignal()("code", "rows_parsed")(currentCount);
            }
            CurrentIdx = -1;
        }

    public:
        virtual void AddRow(const TVector<TStringBuf>& values) override {
            ++CurrentIdx;
            ++CurrentIdxGlobal;
            const ui32 currentCount = GetCurrentRecordsCount();
            RecordsPool[currentCount - 1] = values;
            if (currentCount == MTParsingPoolSize) {
                ParseMT(currentCount);
            }
        }

        virtual void Initialize(const ui32 /*recordsCount*/, const TVector<NCS::NStorage::TOrderedColumn>& orderedColumns) override {
            Lock = nullptr;
            Session->Commit();
            TMap<TString, ui32> remapColumns;
            ui32 idx = 0;
            for (auto&& i : orderedColumns) {
                remapColumns[i.GetName()] = idx++;
            }
            Decoder = TDecoder(remapColumns);
        }

        ui32 GetCurrentRecordsCount() const {
            if (CurrentIdx == -1) {
                return 0;
            }
            return CurrentIdx % (MTParsingPoolSize)+1;
        }

        void Finish() {
            TTimeGuard tg("parsing last " + ToString(GetCurrentRecordsCount()) + " records (" + Owner.GetTableName() + ")");
            const ui32 currentCount = GetCurrentRecordsCount();
            if (currentCount > MTParsingPoolSize * 0.5) {
                ParseMT(currentCount);
            } else {
                ParseST(currentCount);
            }
            Owner.CleanCurrentBuffer(Locked);
            auto expiredEvents = Owner.CleanExpiredEvents(Now() - Owner.Config.GetEventsStoreInMemDeep());
            if (!expiredEvents.empty()) {
                DEBUG_LOG << "Cleaned " << expiredEvents.size() << " expired events" << Endl;
            }
            if (IterationIdx != 1) {
                Queue->Stop();
            }
        }
    };

protected:
    virtual void OnFinishCacheModification(const TParsedObjects& /*records*/) const {
    }

    bool FinishCacheModification(TParsedObjects& records) const {
        DEBUG_LOG << "FinishCacheModification for " << GetTableName() << " START..." << records.GetCurrentRecordsCount() << Endl;
        records.Finish();
        DEBUG_LOG << "FinishCacheModification for " << GetTableName() << " PARSING FINISHED..." << records.GetCurrentRecordsCount() << Endl;
        OnFinishCacheModification(records);
        if (records.GetCurrentIdxGlobal()) {
            SendGlobalMessage<TNotifyHistoryChanged>(GetTableName(), records.GetCurrentIdxGlobal(), records.GetActualInstant());
        }
        CurrentInstant = records.GetActualInstant();
        DEBUG_LOG << "FinishCacheModification for " << GetTableName() << " FINISHED" << Endl;
        return true;
    }

    virtual void FillStaticDeepEventsConditions(THistoryConditionsTimeline& /*result*/) const {}

    virtual void FillAdditionalEventsConditions(TMap<TString, TSet<TString>>& /*result*/, const ui64 /*historyEventIdMaxWithFinishInstant*/) const {
    }

    TString BuildAdditionalEventsConditions(const ui64 historyEventIdMaxWithFinishInstant) const {
        TMap<TString, TSet<TString>> result;
        FillAdditionalEventsConditions(result, historyEventIdMaxWithFinishInstant);
        TStringStream ss;
        TStringBuilder sbLog;
        for (auto&& i : result) {
            if (!ss.Empty()) {
                ss << " OR ";
            }
            ss << i.first << " IN ('" << JoinSeq("','", i.second) << "')";
            sbLog << "ADDITIONAL_EVENTS_INFO: " << i.first << ":" << i.second.size() << ";";
        }
        INFO_LOG << sbLog << Endl;
        return ss.Str();
    }

    virtual void BuildCache() const noexcept {
        StartCacheConstruction();
        TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("CacheConstructionFor:" + GetTableName());
        TDBEventLogGuard dbTg(Database, "download " + GetTableName());
        INFO_LOG << "RESOURCE: Start cache construction for " << GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
        TWriteGuard rg(MutexRequest);
        const TInstant finishInstant = Min(ModelingNow() - TDuration::Minutes(5), Config.GetLastInstant());
        StartInstant = Min(ModelingNow() - Config.GetDeep(), finishInstant);
        const TInstant deepStartInstant = Min(ModelingNow() - Config.GetMaxHistoryDeep(), StartInstant);

        const i64 historyEventIdMinDeep = GetHistoryEventIdByTimestamp(deepStartInstant, "deep event");
        const i64 historyEventIdMin = GetHistoryEventIdByTimestamp(StartInstant, "deepest event");
        i64 historyEventIdMax = GetHistoryEventIdByTimestamp(finishInstant, "fresh event");
        i64 historyEventIdMaxWithFinishInstant = historyEventIdMax;
        Y_ASSERT(historyEventIdMinDeep <= historyEventIdMin);
        Y_ASSERT(historyEventIdMin <= historyEventIdMax);

        while (historyEventIdMax > historyEventIdMin && (Events.empty() || Events.back()->GetHistoryInstant() > StartInstant)) {
            const i64 historyEventIdNext = Max(historyEventIdMin, historyEventIdMax - (i64)Config.GetChunkSize());
            TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("iteration for history loading from " + ::ToString(historyEventIdNext) + " to " + ToString(historyEventIdMax) + " for " + GetTableName());
            TParsedObjects records(*this, TInstant::Zero(), TInstant::Zero(), true, true, true);
            records.SetLocked(true);
            auto transaction = records.GetSession().GetTransaction();
            auto result = transaction->Exec("SELECT " + records.GetDecoder().GetFieldsForRequest() + " FROM " + GetTableName()
                + " WHERE " + GetSeqFieldName() + " > " + ToString(historyEventIdNext)
                + " AND " + GetSeqFieldName() + " <= " + ToString(historyEventIdMax)
                + " ORDER BY " + GetSeqFieldName() + " DESC", &records);
            CHECK_WITH_LOG(result->IsSucceed()) << transaction->GetStringReport() << Endl;
            CHECK_WITH_LOG(FinishCacheModification(records));
            historyEventIdMax = historyEventIdNext;
        }

        THistoryConditionsTimeline historyTimeline(Config.GetMaxHistoryDeep(), Config.GetUseDeepTagsLoading(), Config.GetUseUndeadTagsLoading());
        FillStaticDeepEventsConditions(historyTimeline);

        TMap<i64, TString> conditions;
        for (auto&& [delta, condition] : historyTimeline.BuildFinalConditions()) {
            i64 eventId = GetHistoryEventIdByTimestamp(finishInstant - delta, "deep interval");
            if (eventId > historyEventIdMax) {
                continue;
            }
            conditions[eventId] = condition;
        }

        auto condInterval = conditions.rbegin();
        TString deepTags = condInterval == conditions.rend() ? "" : condInterval->second;
        while (historyEventIdMax >= 0 && (Events.empty() || Events.back()->GetHistoryInstant() > deepStartInstant)) {
            const TString additionalTags = BuildAdditionalEventsConditions(historyEventIdMaxWithFinishInstant);
            if (!additionalTags) {
                break;
            }
            i64 historyEventIdNext = historyEventIdMax - (i64)Config.GetChunkSize();
            if (condInterval != conditions.rend() && condInterval->first > historyEventIdNext) {
                historyEventIdNext = condInterval->first;
                deepTags = condInterval->second;
                condInterval++;
            }
            TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("iteration for history deep loading from " + ::ToString(historyEventIdNext) + " to " + ToString(historyEventIdMax) + "/" + deepTags + " + " + additionalTags + " for " + GetTableName());
            TParsedObjects records(*this, TInstant::Zero(), TInstant::Zero(), true, true, true);
            records.SetLocked(true);
            auto transaction = records.GetSession().GetTransaction();
            auto result = transaction->Exec("SELECT " + records.GetDecoder().GetFieldsForRequest() + " FROM " + GetTableName()
                + " WHERE " + GetSeqFieldName() + " > " + ToString(historyEventIdNext)
                + " AND " + GetSeqFieldName() + " <= " + ToString(historyEventIdMax)
                + " AND (" + deepTags + (deepTags ? " OR " : "") + additionalTags + ")"
                + " ORDER BY " + GetSeqFieldName() + " DESC", &records);
            CHECK_WITH_LOG(result->IsSucceed()) << transaction->GetStringReport() << Endl;
            CHECK_WITH_LOG(FinishCacheModification(records));
            historyEventIdMax = historyEventIdNext;
        }

        while (condInterval != conditions.rend()) {
            const i64 historyEventIdNext = condInterval->first;
            deepTags = condInterval->second;
            TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("iteration for history deep loading from " + ::ToString(historyEventIdNext) + " to " + ToString(historyEventIdMax) + "/" + deepTags + " for " + GetTableName());
            TParsedObjects records(*this, TInstant::Zero(), TInstant::Zero(), true, true, true);
            records.SetLocked(true);
            auto transaction = records.GetSession().GetTransaction();
            auto result = transaction->Exec("SELECT " + records.GetDecoder().GetFieldsForRequest() + " FROM " + GetTableName()
                + " WHERE " + GetSeqFieldName() + " <= " + ToString(historyEventIdMax)
                + " AND " + GetSeqFieldName() + " > " + ToString(historyEventIdNext)
                + " AND " + deepTags
                + " ORDER BY " + GetSeqFieldName() + " DESC", &records);
            CHECK_WITH_LOG(result->IsSucceed()) << transaction->GetStringReport() << Endl;
            CHECK_WITH_LOG(FinishCacheModification(records));
            historyEventIdMax = historyEventIdNext;
            condInterval++;
        }

        std::reverse(Events.begin(), Events.end());
        FinishCacheConstruction();
        if (historyEventIdMaxWithFinishInstant != -1) {
            LockedMaxEventId = historyEventIdMaxWithFinishInstant;
        }
        INFO_LOG << "RESOURCE: Finish cache construction for " << GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
    }

    bool RefreshCache(const TInstant reqActuality, const bool doFreezeExt, const bool ignoreParseErrors) const {
        const auto gLogging = TFLRecords::StartContext()("locked_max_id", LockedMaxEventId)("table_name", GetTableName());
        const bool doFreeze = doFreezeExt && (Now() - LastSuccessReadInstant) < Config.GetMaximalLockedIntervalDuration();
        DEBUG_LOG << "RefreshCache for " << GetTableName() << " START..." << Endl;
        TInstant actualInstant = Min(Now(), reqActuality);
        TInstant modelingActualInstant = ModelingNow();

        if (Config.GetLastInstant() < TInstant::Max()) {
            TFLEventLog::Debug("Skip update")("reason", "LastInstant limit");
            return true;
        }

        if (!actualInstant) {
            return true;
        }

        if (CurrentInstant > actualInstant && !doFreeze) {
            TFLEventLog::Debug("Skip update")("reason", "current > reqActuality")("current", CurrentInstant)("req_actuality", actualInstant);
            return true;
        }

        TWriteGuard rg(MutexRequest);
        if (CurrentInstant > actualInstant && !doFreeze) {
            TFLEventLog::Debug("Skip update after lock")("reason", "current > reqActuality")("current", CurrentInstant)("req_actuality", actualInstant);
            return true;
        }
        const bool readFull = LockedMaxEventId == TEvent::IncorrectEventId;
        auto records = MakeHolder<TParsedObjects>(*this, actualInstant, modelingActualInstant, !doFreeze, ignoreParseErrors);
        records->SetLocked(doFreeze);
        {
            auto transaction = records->GetSession().GetTransaction();
            TString selectRequest;
            if (readFull) {
                selectRequest = "SELECT * FROM " + GetTableName() + " ORDER BY " + GetSeqFieldName();
            } else {
                selectRequest = "SELECT * FROM " + GetTableName() + " WHERE " + GetSeqFieldName() + " > " + ToString(LockedMaxEventId) + " ORDER BY " + GetSeqFieldName();
            }
            if (doFreeze) {
                const TInstant lockInstant = ModelingNow();
                auto trResult = transaction->MultiExec({
                    {"LOCK TABLE " + GetTableName() + " IN SHARE MODE NOWAIT"},
                    {selectRequest, *records},
                    {"COMMIT"}
                });
                if (!trResult) {
                    records = MakeHolder<TParsedObjects>(*this, actualInstant, modelingActualInstant, true, ignoreParseErrors);
                    records->SetLocked(false);
                    transaction = records->GetSession().GetTransaction();
                    TFLEventLog::Debug("Lock failed. Reask withno lock");
                    if (!transaction->Exec(selectRequest, records.Get())) {
                        return false;
                    }
                } else {
                    LastSuccessLockInstant = lockInstant;
                    TFLEventLog::Debug("Lock succeed. Cursor moved.").SetPriority(ELogPriority::TLOG_RESOURCES);
                }
            } else {
                if (!transaction->Exec(selectRequest, records.Get())) {
                    return false;
                }
            }

            LastSuccessReadInstant = Now();
        }
        return FinishCacheModification(*records);
    }

public:

    virtual bool IsNotUniqueTableSequential() const {
        return false;
    }

    virtual bool Refresh() override {
        return UpdateFreeze(Now());
    }

    TCSSignals::TSignalBuilder LCacheSignal() const {
        return std::move(TCSSignals::LSignal("history_freshness")("table", GetTableName()));
    }

    TCSSignals::TSignalBuilder CacheSignal() const {
        return std::move(TCSSignals::Signal("history_freshness")("table", GetTableName()));
    }

    virtual bool MetricSignal() override {
        LCacheSignal()("code", "freshness")((Now() - CurrentInstant).MilliSeconds());
        return true;
    }

    ui64 GetInsertedEventsCount() const {
        return AtomicGet(InsertedEventsCount);
    }

    TEventId GetCurrentMaxEventId() const {
        return CurrentMaxEventId;
    }

    TEventId GetLockedMaxEventId() const {
        return LockedMaxEventId;
    }

    bool GetLockedMinEventId(const TInstant since, TMaybeFail<ui64>& result, const TInstant reqActuality) const {
        if (LockedMaxEventId == TEvent::IncorrectEventId) {
            return true;
        }
        TEvents events;
        if (!GetEventsAllSafe(since, events, reqActuality)) {
            ERROR_LOG << "cannot fetch events" << Endl;
            return false;
        }

        if (events.empty()) {
            return true;
        }

        result = LockedMaxEventId;
        for (auto&& i : events) {
            if (*result > i->GetHistoryEventId()) {
                result = i->GetHistoryEventId();
            }
        }
        return true;
    }

    TInstant GetCurrentInstant() const {
        return CurrentInstant;
    }

    TEventId GetIncorrectEventId() const {
        return TEvent::IncorrectEventId;
    }

    const THistoryConfig& GetConfig() const {
        return Config;
    }

    bool Update(const TDuration maxAge) const {
        return Update(Now() - maxAge);
    }

    bool NeedUpdate(const TInstant reqActuality) const {
        return CurrentInstant < reqActuality;
    }

    bool Update(const TInstant reqActuality) const {
        const bool isBuilding = AtomicIncrement(UpdatesCounter) == 1;
        if (isBuilding) {
            BuildCache();
        }
        return RefreshCache(reqActuality, false, isBuilding);
    }

    bool UpdateFreeze(const TDuration maxAge) const {
        return UpdateFreeze(Now() - maxAge);
    }

    bool UpdateFreeze(const TInstant reqActuality) const {
        const bool isBuilding = AtomicIncrement(UpdatesCounter) == 1;
        if (isBuilding) {
            BuildCache();
        }
        bool updateSucceed = RefreshCache(reqActuality, Config.GetNeedLock(), isBuilding);
        if (updateSucceed) {
            CacheSignal()("code", "update_success");
        } else {
            CacheSignal()("code", "update_fail");
        }
        return updateSucceed;
    }

    virtual void FillServerInfo(NJson::TJsonValue& report) const {
        report["locked_max_event"] = LockedMaxEventId;
        report["current_max_event"] = CurrentMaxEventId;
        report["start_instant"] = StartInstant.Seconds();
        report["current_instant"] = CurrentInstant.Seconds();
        report["start_instant_hr"] = StartInstant.ToString();
        report["current_instant_hr"] = CurrentInstant.ToString();
        report["events_count"] = Events.size();
    }

    virtual bool Process(IMessage* message) override {
        TCollectServerInfo* messageSI = dynamic_cast<TCollectServerInfo*>(message);
        if (messageSI) {
            NJson::TJsonValue report;
            FillServerInfo(report);
            messageSI->Fields[Name()] = report;
            return true;
        }
        const NFrontend::TCacheRefreshMessage* refreshCache = dynamic_cast<const NFrontend::TCacheRefreshMessage*>(message);
        if (refreshCache && (refreshCache->GetComponents().empty() || refreshCache->GetComponents().contains(GetTableName()))) {
            Update(Now());
            return true;
        }
        TServerStartedMessage* serverStarted = dynamic_cast<TServerStartedMessage*>(message);
        if (serverStarted) {
         //   CHECK_WITH_LOG(serverStarted->MutableSequentialTableNames().emplace(GetTableName()).second || IsNotUniqueTableSequential()) << "table " << GetTableName() << " is used by more than one sequential";
            return true;
        }
        return false;
    }

    virtual TString Name() const override {
        return "SeqTableFor:" + GetTableName();
    }

    virtual ~TBaseSequentialTableImpl() {
        UnregisterGlobalMessageProcessor(this);
    }

    TBaseSequentialTableImpl(const IHistoryContext& context, const TString& tableName, const THistoryConfig& config)
        : IBaseSequentialTableImpl(context.GetDatabase(), tableName)
        , IAutoActualization("auto-refresh-" + tableName, config.GetPingPeriod())
        , DB(context.GetDatabase())
        , Config(config)
        , Context(context)
    {
        StartInstant = ModelingNow() - Config.GetDeep();
        CurrentInstant = StartInstant;
        RegisterGlobalMessageProcessor(this);
    }

    TBaseSequentialTableImpl(NStorage::IDatabase::TPtr db, const TString& tableName, const THistoryConfig& config)
        : IBaseSequentialTableImpl(db, tableName)
        , IAutoActualization("auto-refresh-" + tableName, config.GetPingPeriod())
        , DB(db)
        , Config(config)
        , OwnerContext(MakeHolder<THistoryContext>(db))
        , Context(*OwnerContext)
    {
        StartInstant = ModelingNow() - Config.GetDeep();
        CurrentInstant = StartInstant;
        RegisterGlobalMessageProcessor(this);
    }

    const IHistoryContext& GetContext() const {
        return Context;
    }

    bool GetEventsImpl(const TDeque<TEventPtr>& events, const TInstant startInstant, TEvents& results) const {
        results.clear();
        for (auto it = events.rbegin(); it != events.rend(); ++it) {
            if ((*it)->GetHistoryInstant() < startInstant)
                break;
            results.emplace_back(*it);
        }
        std::reverse(results.begin(), results.end());
        return true;
    }

    bool GetEventsSinceIdImpl(const TDeque<TEventPtr>& events, TEventId id, TEvents& results) const {
        Y_ASSERT(std::is_sorted(events.begin(), events.end(), [](const TAtomicSharedPtr<TEvent>& left, const TAtomicSharedPtr<TEvent>& right) {
            return left->GetHistoryEventId() < right->GetHistoryEventId();
        }));
        auto lower = std::lower_bound(events.begin(), events.end(), id, [](const TAtomicSharedPtr<TEvent>& ev, const TEventId id) {
            return ev->GetHistoryEventId() < id;
        });
        results.assign(lower, events.end());
        return true;
    }

    bool GetEventsImpl(const TDeque<TEventPtr>& events, TEventId id, TEvents& results) const {
        Y_ASSERT(std::is_sorted(events.begin(), events.end(), [](const TAtomicSharedPtr<TEvent>& left, const TAtomicSharedPtr<TEvent>& right) {
            return left->GetHistoryEventId() < right->GetHistoryEventId();
        }));
        auto lower = std::lower_bound(events.begin(), events.end(), id, [](const TAtomicSharedPtr<TEvent>& ev, TEventId id) {
            return ev->GetHistoryEventId() < id;
        });
        auto upper = std::upper_bound(events.begin(), events.end(), id, [](TEventId id, const TAtomicSharedPtr<TEvent>& ev) {
            return id < ev->GetHistoryEventId();
        });
        results.assign(lower, upper);
        return true;
    }

    std::pair<TEvent, bool> GetEvent(const TEventId id, const TDuration maxAge = TDuration::Zero()) const {
        TReadGuard rg(Mutex);
        if (Events.empty() || Events.back()->GetHistoryEventId() < id) {
            rg.Release();
            Update(maxAge);
        }
        TVector<TAtomicSharedPtr<TEvent>> events;
        events.reserve(1);
        {
            TReadGuard rg(Mutex);
            if (!GetEventsImpl(Events, id, events)) {
                return {{}, false};
            }
        }
        switch (events.size()) {
            case 0:
                return {{}, true};
            case 1:
                return {*events[0], true};
            default:
                ERROR_LOG << "get_event: EventId collision: " << id << Endl;
                return {{}, false};
        }
    }

    bool GetEventsSinceId(const TEventId& id, TEvents& results, const TInstant reqActuality) const {
        Update(reqActuality);
        TReadGuard rg(Mutex);
        return GetEventsSinceIdImpl(Events, id, results);
    }

    bool GetEventsAll(const TInstant startInstant, TEvents& results, const TInstant reqActuality) const {
        if (!Update(reqActuality)) {
            return false;
        }
        TReadGuard rg(Mutex);
        return GetEventsImpl(Events, startInstant, results);
    }

    bool GetEventsAllSafe(const TInstant startInstant, TEvents& results, const TInstant reqActuality) const {
        if (!Update(reqActuality)) {
            return false;
        }
        TReadGuard rg(Mutex);
        return GetEventsImpl(Events, startInstant, results);
    }

    bool GetLastEvent(TEvent& result) const {
        TReadGuard rg(Mutex);
        if (!Events.size()) {
            return false;
        }
        result = *Events.back();
        return true;
    }

    bool RestoreEventsByCondition(const TSRCondition& condition, TVector<TEvent>& events, NCS::TEntitySession& session) const {
        NStorage::TObjectRecordsSet<TEvent> records;
        TSRSelect query(GetTableName(), &records);
        query.SetCondition(condition);
        if (!session.ExecRequest(query)) {
            return false;
        }
        events = records.DetachObjects();
        return true;
    }

    bool RestoreEventsById(const TVector<TEventId>& eventIds, TVector<TEvent>& events, NCS::TEntitySession& session) const {
        if (eventIds.empty()) {
            events.clear();
            return true;
        }
        TSRCondition condition;
        condition.Init<TSRBinary>(GetSeqFieldName(), eventIds);
        return RestoreEventsByCondition(condition, events, session);
    }

    template <class TFilter>
    bool FilterEvents(const TInstant since, const TFilter& filter, TEvents& events, const TInstant reqActuality, const ui32 eventsLimit = 0) const {
        TEvents historyActions;
        if (!GetEventsAll(since, historyActions, reqActuality)) {
            return false;
        }
        for (auto it = historyActions.rbegin(); it != historyActions.rend(); ++it) {
            auto& event = *it;
            if (filter(event)) {
                events.emplace_back(event);
            }
            if (eventsLimit && eventsLimit <= events.size()) {
                break;
            }
        }
        std::reverse(events.begin(), events.end());
        return true;
    }
};

template <class TEvent, class TObjectId, class TCachedObject = typename TEvent::TEntity>
class IHistoryCallback : public TDBEntitiesCacheImpl<TCachedObject, TObjectId> {
private:
    using TBase = TDBEntitiesCacheImpl<TCachedObject, TObjectId>;
    using TIdRef = typename TIdTypeSelector<TObjectId>::TIdRef;
    TSet<ui64> ReadyEvents;
protected:
    using TBase::MutexCachedObjects;
    ui64 LockedMaxEventId = TEvent::IncorrectEventId;
    virtual bool DoAcceptHistoryEventUnsafe(const TAtomicSharedPtr<TEvent>& dbEvent, const bool isNewEvent) = 0;
    virtual TIdRef GetEventObjectId(const TEvent& ev) const = 0;
public:
    using TPtr = TAtomicSharedPtr<IHistoryCallback>;
    virtual bool Accept(const TDeque<TAtomicSharedPtr<TEvent>>& events) {
        TSet<TIdRef> eventObjectIds;
        TWriteGuard wg(MutexCachedObjects);
        for (auto&& i : events) {
            TIdRef eventObjectId = GetEventObjectId(*i);
            const bool isNewEvent = !ReadyEvents.contains(i->GetHistoryEventId());
            if (!isNewEvent && !eventObjectIds.contains(eventObjectId)) {
                continue;
            }
            ReadyEvents.emplace(i->GetHistoryEventId());
            eventObjectIds.emplace(eventObjectId);
            DoAcceptHistoryEventUnsafe(i, isNewEvent);
        }
        return true;
    }

    void SetLockedInfo(const ui64 lockedMaxEventId) {
        if (lockedMaxEventId != TEvent::IncorrectEventId) {
            LockedMaxEventId = lockedMaxEventId;
            if (ReadyEvents.empty() || *ReadyEvents.rbegin() <= lockedMaxEventId) {
                ReadyEvents.clear();
            } else {
                ReadyEvents.erase(ReadyEvents.begin(), ReadyEvents.upper_bound(lockedMaxEventId));
            }
        }
    }
};

template <class TEvent, class TObjectId = TStringBuf, class TCachedObject = typename TEvent::TEntity>
class TCallbackSequentialTableImpl: public TBaseSequentialTableImpl<TEvent> {
private:
    using TBase = TBaseSequentialTableImpl<TEvent>;
    using IHistoryCallback = IHistoryCallback<TEvent, TObjectId, TCachedObject>;
    TVector<IHistoryCallback*> Callbacks;
protected:
    using TBase::Config;
    using TBase::StartInstant;
    using TBase::FinishCacheConstruction;
    using TBase::GetHistoryEventIdByTimestamp;
    using TBase::LockedMaxEventId;
    using TBase::Events;
    using TBase::ReadyEvents;
    using TBase::GetCallbackNeedsHistoryLoad;
    virtual void BuildCache() const noexcept override {
        if (GetCallbackNeedsHistoryLoad()) {
            TBase::BuildCache();
            return;
        }
        StartInstant = Min(ModelingNow() - TDuration::Minutes(5), Config.GetLastInstant());
        const i64 historyEventIdMin = GetHistoryEventIdByTimestamp(StartInstant, "deepest event");
        FinishCacheConstruction();
        if (historyEventIdMin != -1) {
            LockedMaxEventId = historyEventIdMin;
        }
    }

    virtual void CleanCurrentBuffer(const bool locked) const override {
        for (auto&& c : Callbacks) {
            c->Accept(Events);
            c->SetLockedInfo(LockedMaxEventId);
        }
        TBase::CleanCurrentBuffer(locked);
        TBaseSequentialTableImpl<TEvent>::PopFrontEvent(Events, LockedMaxEventId);
    }

public:
    using TBase::TBase;

    void RegisterCallback(IHistoryCallback* callback) {
        Callbacks.emplace_back(callback);
    }

    void UnregisterCallback(IHistoryCallback* callback) {
        Callbacks.erase(std::remove_if(Callbacks.begin(), Callbacks.end(), [callback](const IHistoryCallback* item) {return callback == item; }), Callbacks.end());
    }

};

template <class TEvent>
class IEventsIndexWriter {
public:
    virtual void FillServerInfo(NJson::TJsonValue& report) const = 0;
    virtual void Rebuild(const TDeque<TAtomicSharedPtr<TEvent>>& events) = 0;
    virtual void InsertSorted(TAtomicSharedPtr<TEvent> ev) = 0;
    virtual void PopFrontEvent(TAtomicSharedPtr<TEvent> ev) = 0;
};

template <class TEvent>
class IIndexRefreshAgent {
public:
    virtual bool RefreshIndexData(const TInstant reqActuality) = 0;
    virtual bool RegisterIndex(IEventsIndexWriter<TEvent>* index) = 0;
    virtual bool UnregisterIndex(IEventsIndexWriter<TEvent>* index) = 0;
    virtual TReadGuard StartRead() = 0;
};

template <class T>
class TKeyIdAdapter {
public:
};

template <>
class TKeyIdAdapter<TStringBuf> {
public:
    using TKeyId = TString;
    static TKeyId MakeIndexKey(const TStringBuf id) {
        return TKeyId(id);
    }
};

template <>
class TKeyIdAdapter<TString> {
public:
    using TKeyId = TString;
    static TKeyId MakeIndexKey(const TString& id) {
        return TKeyId(id);
    }
};

template <>
class TKeyIdAdapter<ui32> {
public:
    using TKeyId = ui32;
    static ui32 MakeIndexKey(const ui32 id) {
        return id;
    }
};

template <>
class TKeyIdAdapter<ui64> {
public:
    using TKeyId = ui64;
    static ui64 MakeIndexKey(const ui64 id) {
        return id;
    }
};

template <>
class TKeyIdAdapter<i64> {
public:
    using TKeyId = i64;
    static i64 MakeIndexKey(const i64 id) {
        return id;
    }
};

template <class TEvent, class TKeyIdExt = TStringBuf>
class IEventsIndex: public IEventsIndexWriter<TEvent> {
public:
    using TKeyIdInt = typename TKeyIdAdapter<TKeyIdExt>::TKeyId;
private:
    using IIndexRefreshAgent = IIndexRefreshAgent<TEvent>;
protected:
    TMap<TKeyIdInt, TDeque<TAtomicSharedPtr<TEvent>>> Index;
    IIndexRefreshAgent* IndexRefreshAgent = nullptr;
    TRWMutex MutexIndex;
    TString Name = "undefined";

    virtual TMaybe<TKeyIdExt> ExtractKey(TAtomicSharedPtr<TEvent> ev) const = 0;
private:
    template <class T>
    static ui64 GetIdHash(const T id) {
        return id;
    }

    static ui64 GetIdHash(const TStringBuf id) {
        return FnvHash<ui64>(id.data(), id.size());
    }

    static ui64 GetIdHash(const TString& id) {
        return FnvHash<ui64>(id.data(), id.size());
    }

    class TSortedEvent {
        RTLINE_READONLY_ACCEPTOR_DEF(Id, TKeyIdExt);
        RTLINE_READONLY_ACCEPTOR_DEF(Idx, ui32);
        ui64 Hash;
        TAtomicSharedPtr<TEvent> Object;

    public:
        TAtomicSharedPtr<TEvent> GetObject() const {
            return Object;
        }

        TSortedEvent(const TKeyIdExt& id, const ui32 idx, TAtomicSharedPtr<TEvent> object)
            : Id(id)
            , Idx(idx)
            , Object(object) {
            Hash = GetIdHash(Id);
        }

        bool operator<(const TSortedEvent& item) const {
            if (Hash < item.Hash) {
                return true;
            } else if (Hash > item.Hash) {
                return false;
            } else {
                return (Id < item.Id) || (Id == item.Id && Idx < item.Idx);
            }
        }
    };

    virtual void DoInsertSorted(TDeque<TAtomicSharedPtr<TEvent>>& v, TAtomicSharedPtr<TEvent> ev) {
        TBaseSequentialTableImpl<TEvent>::InsertSorted(v, ev);
    }

    virtual void DoPopFrontEvent(TDeque<TAtomicSharedPtr<TEvent>>& v, TAtomicSharedPtr<TEvent> ev) const {
        TBaseSequentialTableImpl<TEvent>::PopFrontEvent(v, ev->GetHistoryEventId());
    }

public:

    IEventsIndex(IIndexRefreshAgent* agent, const TString& name)
        : IndexRefreshAgent(agent)
        , Name(name)
    {
        IndexRefreshAgent->RegisterIndex(this);
    }

    virtual ~IEventsIndex() {
        IndexRefreshAgent->UnregisterIndex(this);
    }

    virtual void FillServerInfo(NJson::TJsonValue& report) const override {
        report["events_index_info_" + Name + "_keys_count"] = Index.size();
    }

    TMap<TKeyIdInt, TDeque<TAtomicSharedPtr<TEvent>>> GetCachedEvents() const {
        TReadGuard gMutex = IndexRefreshAgent->StartRead();
        return Index;
    }

    virtual void PopFrontEvent(TAtomicSharedPtr<TEvent> ev) override {
        auto key = ExtractKey(ev);
        if (key) {
            auto it = Index.find(*key);
            if (it != Index.end()) {
                DoPopFrontEvent(it->second, ev);
                if (it->second.empty()) {
                    Index.erase(it);
                }
            }
        }
    };

    virtual void InsertSorted(TAtomicSharedPtr<TEvent> ev) override {
        auto key = ExtractKey(ev);
        if (key) {
            auto it = Index.find(*key);
            if (it == Index.end()) {
                it = Index.emplace(TKeyIdAdapter<TKeyIdExt>::MakeIndexKey(*key), TDeque<TAtomicSharedPtr<TEvent>>()).first;
            }
            DoInsertSorted(it->second, ev);
        }
    }

    bool GetLastEventById(const TKeyIdExt& id, TAtomicSharedPtr<TEvent>& result, const TInstant reqActuality) const {
        if (!IndexRefreshAgent->RefreshIndexData(reqActuality)) {
            return false;
        }
        TReadGuard gMutex = IndexRefreshAgent->StartRead();
        auto it = Index.find(id);
        if (it == Index.end() || it->second.empty()) {
            result = nullptr;
        } else {
            result = it->second.back();
        }
        return true;
    }

    TAtomicSharedPtr<TEvent> GetLastEventById(const TKeyIdExt& id, const TInstant until, const bool include) const {
        TReadGuard gMutex = IndexRefreshAgent->StartRead();
        auto it = Index.find(id);
        if (it == Index.end() || it->second.empty()) {
            return nullptr;
        }
        for (auto itEvent = it->second.rbegin(); itEvent != it->second.rend(); ++itEvent) {
            if ((*itEvent)->GetHistoryInstant().Seconds() < until.Seconds()) {
                return *itEvent;
            } else if ((*itEvent)->GetHistoryInstant().Seconds() == until.Seconds() && include) {
                return *itEvent;
            }
        }
        return nullptr;
    }

    size_t GetCacheEventsCount(const TKeyIdExt& keyId) const {
        TReadGuard gMutex = IndexRefreshAgent->StartRead();
        auto it = Index.find(keyId);
        if (it == Index.end()) {
            return 0;
        } else {
            return it->second.size();
        }
    }

    bool GetEvents(const TKeyIdExt& keyId, const TInstant since, TVector<TAtomicSharedPtr<TEvent>>& results, const TInstant reqActuality) const {
        CHECK_WITH_LOG(IndexRefreshAgent);
        if (!IndexRefreshAgent->RefreshIndexData(reqActuality)) {
            return false;
        }
        results.clear();
        TReadGuard gMutex = IndexRefreshAgent->StartRead();
        auto itKey = Index.find(keyId);
        if (itKey == Index.end()) {
            return true;
        }
        for (auto it = itKey->second.rbegin(); it != itKey->second.rend(); ++it) {
            if ((*it)->GetHistoryInstant() < since)
                break;
            results.emplace_back(*it);
        }
        std::reverse(results.begin(), results.end());
        return true;
    }

    template <class TSearchPred>
    bool FindEvent(const TKeyIdExt& keyId, TAtomicSharedPtr<TEvent> result, const TSearchPred& searchPred, const TInstant reqActuality) const {
        CHECK_WITH_LOG(IndexRefreshAgent);
        if (!IndexRefreshAgent->RefreshIndexData(reqActuality)) {
            return false;
        }
        TReadGuard gMutex = IndexRefreshAgent->StartRead();
        auto itKey = Index.find(keyId);
        if (itKey == Index.end()) {
            return true;
        }
        for (auto it = itKey->second.rbegin(); it != itKey->second.rend(); ++it) {
            const int val = searchPred(*it);
            if (val == 0) {
                result = *it;
                return true;
            } else if (val < 0) {
                return true;
            }
        }
        return true;
    }

    bool FindRevision(const TKeyIdExt& keyId, TAtomicSharedPtr<TEvent> result, const ui64 revision, const TInstant reqActuality) const {
        const auto pred = [revision](const TAtomicSharedPtr<TEvent>& ev) {
            return (i64)ev->GetRevision() - (i64)revision;
        };
        return FindEvent(keyId, result, pred, reqActuality);
    }

    bool GetEvents(const TKeyIdExt& keyId, TDeque<TAtomicSharedPtr<TEvent>>& results, const TInstant reqActuality) const {
        CHECK_WITH_LOG(IndexRefreshAgent);
        if (!IndexRefreshAgent->RefreshIndexData(reqActuality)) {
            return false;
        }
        TReadGuard gMutex = IndexRefreshAgent->StartRead();
        auto itKey = Index.find(keyId);
        if (itKey == Index.end()) {
            results.clear();
            return true;
        }
        results = itKey->second;
        return true;
    }

    bool GetEvents(const TSet<TKeyIdExt>& keyIds, TVector<TAtomicSharedPtr<TEvent>>& results, const TInstant reqActuality) const {
        CHECK_WITH_LOG(IndexRefreshAgent);
        if (!IndexRefreshAgent->RefreshIndexData(reqActuality)) {
            return false;
        }
        TVector<TAtomicSharedPtr<TEvent>> resultsLocal;
        {
            TReadGuard gMutex = IndexRefreshAgent->StartRead();
            for (auto&& i : keyIds) {
                auto itKey = Index.find(i);
                if (itKey == Index.end()) {
                    continue;
                }
                resultsLocal.insert(resultsLocal.end(), itKey->second.begin(), itKey->second.end());
            }
        }
        const auto pred = [](TAtomicSharedPtr<TEvent> l, TAtomicSharedPtr<TEvent> r) {
            return l->GetHistoryEventId() < r->GetHistoryEventId();
        };

        std::sort(resultsLocal.begin(), resultsLocal.end(), pred);
        std::swap(results, resultsLocal);
        return true;
    }

    virtual void Rebuild(const TDeque<TAtomicSharedPtr<TEvent>>& events) override {
        TVector<TSortedEvent> evSorted;
        for (auto&& i : events) {
            TMaybe<TKeyIdExt> keyId = ExtractKey(i);
            if (keyId) {
                evSorted.emplace_back(*keyId, evSorted.size(), i);
            }
        }
        std::sort(evSorted.begin(), evSorted.end());

        TKeyIdExt currentId;
        bool isFirst = true;
        TDeque<TAtomicSharedPtr<TEvent>>* vEvents = nullptr;
        for (auto&& i : evSorted) {
            if (isFirst || currentId != i.GetId()) {
                vEvents = &Index.emplace(TKeyIdAdapter<TKeyIdExt>::MakeIndexKey(i.GetId()), TDeque<TAtomicSharedPtr<TEvent>>()).first->second;
                currentId = i.GetId();
                isFirst = false;
            }
            vEvents->emplace_back(i.GetObject());
        }
    }
};

template <class TEvent>
class TIndexedSequentialTableImpl: public TBaseSequentialTableImpl<TEvent>, public IIndexRefreshAgent<TEvent> {
private:
    using TBase = TBaseSequentialTableImpl<TEvent>;
    using IEventsIndexWriter = IEventsIndexWriter<TEvent>;
public:
    using TEventsView = TVector<TAtomicSharedPtr<TEvent>>;
    using TBase::Mutex;
    using TBase::Update;
    using TBase::TBase;
    using TBase::GetTableName;
private:
    mutable TVector<IEventsIndexWriter*> Indexes;

    virtual bool RegisterIndex(IEventsIndexWriter* index) override {
        TWriteGuard wg(Mutex);
        for (auto&& i : Indexes) {
            if (i == index) {
                return false;
            }
        }
        Indexes.emplace_back(index);
        return true;
    }

    virtual bool UnregisterIndex(IEventsIndexWriter* index) override {
        TWriteGuard wg(Mutex);
        const auto pred = [index](IEventsIndexWriter* i) {
            return i == index;
        };
        Indexes.erase(std::remove_if(Indexes.begin(), Indexes.end(), pred), Indexes.end());
        return true;
    }
protected:
    using TBase::Events;

    TVector<TAtomicSharedPtr<TEvent>> CleanExpiredEvents(const TInstant deadline) const override {
        TVector<TAtomicSharedPtr<TEvent>> result;
        if (deadline > TInstant::Zero()) {
            ui64 maxHistoryEventId = 0;
            for (auto&& i : Events) {
                if (i->GetHistoryInstant() < deadline) {
                    maxHistoryEventId = i->GetHistoryEventId();
                    for (auto&& idx : Indexes) {
                        idx->PopFrontEvent(i);
                    }
                } else {
                    break;
                }
            }
            if (maxHistoryEventId) {
                result = TBaseSequentialTableImpl<TEvent>::PopFrontEvent(Events, maxHistoryEventId);
            }
        }
        return result;
    }

    virtual TReadGuard StartRead() override {
        return TReadGuard(Mutex);
    }
    virtual bool RefreshIndexData(const TInstant reqActuality) override {
        return Update(reqActuality);
    }

    virtual void FillServerInfo(NJson::TJsonValue& report) const override {
        TBase::FillServerInfo(report);
        for (auto&& i : Indexes) {
            i->FillServerInfo(report);
        }
    }

    virtual void AddEventDeep(TAtomicSharedPtr<TEvent> /*eventObj*/) const {
    }

    virtual bool DoAddEvent(TAtomicSharedPtr<TEvent>& currentEvent, const bool /*locked*/) const override {
        AddEventDeep(currentEvent);
        for (auto&& i : Indexes) {
            i->InsertSorted(currentEvent);
        }
        return true;
    }

    virtual void FinishCacheConstruction() const override {
        TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("FinishCacheConstruction for " + GetTableName() + "/" + ::ToString(Events.size()) + ")");
        INFO_LOG << "RESOURCE: FinishCacheConstruction start for " << GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
        TBase::FinishCacheConstruction();

        auto pool = CreateThreadPool(Indexes.size());
        TVector<NThreading::TFuture<void>> futures;
        for (auto&& index : Indexes) {
            futures.emplace_back(NThreading::Async([this, index]() {
                index->Rebuild(Events);
            }, *pool));
        }

        NThreading::WaitExceptionOrAll(futures).Wait();

        for (auto&& i : Events) {
            AddEventDeep(i);
        }
        INFO_LOG << "RESOURCE: FinishCacheConstruction finish for " << GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
    }
public:

};
