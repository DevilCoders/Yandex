#pragma once

#include "common.h"
#include "db_entities_history.h"
#include "db_owner.h"
#include "sequential.h"
#include <kernel/common_server/util/algorithm/container.h>
#include <util/random/random.h>
#include "db_direct.h"

template <class TExternalHistoryManager, class TExternalEntity, class TExternalIdType = TString>
class TDBCacheWithHistoryOwner: public IMessageProcessor, public TDBEntitiesCacheImpl<TExternalEntity, TExternalIdType>, public NCS::TDBHistoryOwner<TExternalHistoryManager>, public IAutoActualization {
public:
    using TEntity = TExternalEntity;
    using TIdType = TExternalIdType;
protected:
    using TBaseDB = NCS::TDBHistoryOwner<TExternalHistoryManager>;
    using TBaseImpl = TDBEntitiesCacheImpl<TEntity, TIdType>;
    using THistoryManager = TExternalHistoryManager;
    using TIdRef = typename TIdTypeSelector<TIdType>::TIdRef;
    using TBaseImpl::MutexCachedObjects;
    using TBaseImpl::Objects;
    using TBaseDB::HistoryManager;

    virtual TDuration GetDefaultWaitingDuration() const {
        return TDuration::Seconds(1);
    }
private:
    mutable ui64 InsertedEventsCount = Max<ui64>();
    mutable ui64 LockedHistoryEventId = Max<ui64>();
    mutable TSet<ui64> ReadyEvents;
    mutable TInstant LastRefreshInstant = TInstant::Zero();
    TMutex MutexRebuild;
    TAutoEvent EventWaitingHistory;
protected:
    NStorage::IDatabase::TPtr HistoryCacheDatabase;

    bool RebuildCache(const TInstant reqActuality) const {
        const TSet<TString> deps = GetDependencesBefore();
        for (auto&& i : deps) {
            SendGlobalMessage<TBeforeCacheInvalidateNotification>(i, HistoryManager->GetCurrentInstant(), HistoryManager->GetTableName());
        }
        if (!HistoryManager->Update(reqActuality)) {
            return false;
        }
        INFO_LOG << "RESOURCE: Start snapshot construction for " << HistoryManager->GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
        bool result = false;
        {
            TWriteGuard g(MutexCachedObjects);
            const ui64 insertedEventsCount = HistoryManager->GetInsertedEventsCount();
            const ui64 lockedHistoryEventId = HistoryManager->GetLockedMaxEventId();
            const TInstant lastRefreshInstant = HistoryManager->GetCurrentInstant();

            Objects.clear();
            result = DoRebuildCacheUnsafe();

            if (result) {
                if (lockedHistoryEventId != HistoryManager->GetIncorrectEventId()) {
                    InsertedEventsCount = insertedEventsCount;
                    LockedHistoryEventId = lockedHistoryEventId;
                    LastRefreshInstant = lastRefreshInstant;
                } else {
                    InsertedEventsCount = 0;
                    LockedHistoryEventId = 0;
                    LastRefreshInstant = TInstant::Zero();
                }
            }
            INFO_LOG << "RESOURCE: Finish snapshot construction for " << HistoryManager->GetTableName() << ": " << TRusage::GetCurrentRSS() / 1024 / 1024 << "Mb" << Endl;
        }
        {
            const TSet<TString> deps = GetDependencesAfter();
            for (auto&& i : deps) {
                INFO_LOG << "send TAfterCacheInvalidateNotification for " << i << Endl;
                SendGlobalMessage<TAfterCacheInvalidateNotification>(i, HistoryManager->GetCurrentInstant(), HistoryManager->GetTableName());
            }
        }
        return result;
    }

    virtual bool Process(IMessage* message) override {
        const TServerStartedMessage* activityChecker = dynamic_cast<const TServerStartedMessage*>(message);
        if (activityChecker) {
            CHECK_WITH_LOG(IsActive()) << HistoryManager->GetTableName() << " disabled" << Endl;
            return true;
        }

        const TCommonCacheInvalidateNotification* notifyHistoryInvalidate = dynamic_cast<const TCommonCacheInvalidateNotification*>(message);
        if (notifyHistoryInvalidate) {
            if (notifyHistoryInvalidate->GetTableName() == HistoryManager->GetTableName()) {
                RefreshCache(notifyHistoryInvalidate->GetReqActuality());
            }
            return true;
        }
        const TNotifyHistoryChanged* notifyHistoryChanged = dynamic_cast<const TNotifyHistoryChanged*>(message);
        if (notifyHistoryChanged) {
            if (notifyHistoryChanged->GetTableName() == HistoryManager->GetTableName()) {
                EventWaitingHistory.Signal();
            }
            return true;
        }
        const NFrontend::TCacheRefreshMessage* dropCache = dynamic_cast<const NFrontend::TCacheRefreshMessage*>(message);
        if (dropCache) {
            if (dropCache->GetComponents().empty() || dropCache->GetComponents().contains(HistoryManager->GetTableName())) {
                INFO_LOG << "Refresh Cache for " << HistoryManager->GetTableName() << Endl;
                RebuildCache(Now());
                INFO_LOG << "Refresh Cache for " << HistoryManager->GetTableName() << " OK" << Endl;
            }
            return true;
        }
        return false;
    }

    virtual TString Name() const override {
        return "composite_cache_history_" + ToString(Now().GetValue()) + "_" + ToString(RandomNumber<ui64>());
    }

    virtual bool DoRebuildCacheUnsafe() const = 0;

    virtual TIdRef GetEventObjectId(const typename THistoryManager::THistoryEvent& ev) const = 0;
    virtual void AcceptHistoryEventUnsafe(const typename THistoryManager::THistoryEvent& ev) const = 0;

    virtual bool Refresh() override {
        if (EventWaitingHistory.WaitD(Now() + GetDefaultWaitingDuration())) {
            return RefreshCache(TInstant::Zero(), false);
        }
        return true;
    }

    virtual bool DoStart() override {
        if (!!HistoryManager) {
            HistoryManager->Start();
        }
        EventWaitingHistory.Signal();
        if (!IAutoActualization::DoStart()) {
            return false;
        }
        return true;
    }

    virtual bool DoStop() override {
        EventWaitingHistory.Signal();
        IAutoActualization::DoStop();
        if (!!HistoryManager) {
            HistoryManager->Stop();
        }
        return true;
    }

public:

    TDBCacheWithHistoryOwner(const IHistoryContext& context, const THistoryConfig& hConfig)
        : TBaseDB(context.GetDatabase(), hConfig)
        , IAutoActualization("cache_refresh_actualization", hConfig.GetPingPeriod()) {
        HistoryCacheDatabase = TBaseDB::GetDatabase();
        RegisterGlobalMessageProcessor(this);
    }

    TDBCacheWithHistoryOwner(THolder<IHistoryContext>&& context, const THistoryConfig& hConfig)
        : TBaseDB(context->GetDatabase(), hConfig)
        , IAutoActualization("cache_refresh_actualization", hConfig.GetPingPeriod())
    {
        HistoryCacheDatabase = TBaseDB::GetDatabase();
        RegisterGlobalMessageProcessor(this);
    }

    TDBCacheWithHistoryOwner(NCS::NStorage::IDatabase::TPtr db, const THistoryConfig& hConfig)
        : TBaseDB(db, hConfig)
        , IAutoActualization("cache_refresh_actualization", hConfig.GetPingPeriod())
    {
        HistoryCacheDatabase = TBaseDB::GetDatabase();
        RegisterGlobalMessageProcessor(this);
    }

    ~TDBCacheWithHistoryOwner() {
        UnregisterGlobalMessageProcessor(this);
    }

    TInstant GetLastSuccessLockInstant() const {
        return HistoryManager->GetLastSuccessLockInstant();
    }

    ui64 GetInsertedEventsCount() const {
        return InsertedEventsCount;
    }

    ui64 GetLockedHistoryEventId() const {
        return LockedHistoryEventId;
    }

    TInstant GetLastRefreshInstant() const {
        return LastRefreshInstant;
    }

    virtual TSet<TString> GetDependencesBefore() const {
        return {};
    }

    virtual TSet<TString> GetDependencesAfter() const {
        return {};
    }

    bool RefreshCache(const TInstant reqActuality, const bool doActualizeHistory = true) const override {
        if (LockedHistoryEventId == Max<ui64>()) {
            TGuard<TMutex> g(MutexRebuild);
            if (LockedHistoryEventId == Max<ui64>()) {
                return RebuildCache(reqActuality);
            }
            return true;
        }
        if (doActualizeHistory) {
            if (LastRefreshInstant >= reqActuality) {
                if (reqActuality != TInstant::Zero()) {
                    DEBUG_LOG << "Skip re-fetch cache for " << HistoryManager->GetTableName() << "/" << LastRefreshInstant << ">=" << reqActuality << Endl;
                }
                return true;
            }
            if (!HistoryManager->Update(reqActuality)) {
                ERROR_LOG << "Cannot re-fetch cache for " << HistoryManager->GetTableName() << Endl;
                return false;
            }
        }
        const ui64 lockedHistoryEventId = HistoryManager->GetLockedMaxEventId();
        const ui64 insertedEventsCount = HistoryManager->GetInsertedEventsCount();
        const TInstant lastRefreshInstant = HistoryManager->GetCurrentInstant();
        if (insertedEventsCount == InsertedEventsCount && (lockedHistoryEventId == LockedHistoryEventId || lockedHistoryEventId == HistoryManager->GetIncorrectEventId())) {
            if (LastRefreshInstant < lastRefreshInstant) {
                TWriteGuard g(MutexCachedObjects);
                LastRefreshInstant = Max(LastRefreshInstant, lastRefreshInstant);
                DEBUG_LOG << "Cache re-fetching for " << HistoryManager->GetTableName() << " is useless but ts moved" << Endl;
            } else {
                DEBUG_LOG << "Cache re-fetching for " << HistoryManager->GetTableName() << " is useless" << Endl;
            }
            return true;
        }

        TVector<TAtomicSharedPtr<typename THistoryManager::THistoryEvent>> events;
        if (!HistoryManager->GetEventsSinceId(LockedHistoryEventId + 1, events, reqActuality)) {
            DEBUG_LOG << "Cannot re-fetch " << HistoryManager->GetTableName() << " cache" << Endl;
            return false;
        }
        if (events.size()) {
            const TSet<TString> deps = GetDependencesBefore();
            for (auto&& i : deps) {
                SendGlobalMessage<TBeforeCacheInvalidateNotification>(i, HistoryManager->GetCurrentInstant(), HistoryManager->GetTableName());
            }
        }
        {
            TWriteGuard g(MutexCachedObjects);
            if (doActualizeHistory && LastRefreshInstant >= reqActuality) {
                if (reqActuality != TInstant::Zero()) {
                    DEBUG_LOG << "Skip re-fetch cache for " << HistoryManager->GetTableName() << "/" << LastRefreshInstant << ">=" << reqActuality << Endl;
                }
                return true;
            }
            TTimeGuard tg("Objects from " + HistoryManager->GetTableName() + " cache re-fetching...");
            TSet<TIdRef> eventObjectIds;
            for (auto&& i : events) {
                TIdRef eventObjectId = GetEventObjectId(*i);
                if (ReadyEvents.contains(i->GetHistoryEventId()) && !eventObjectIds.contains(eventObjectId)) {
                    continue;
                }
                ReadyEvents.emplace(i->GetHistoryEventId());
                eventObjectIds.emplace(eventObjectId);
                AcceptHistoryEventUnsafe(*i);
            }
            if (lockedHistoryEventId != HistoryManager->GetIncorrectEventId() && LockedHistoryEventId < lockedHistoryEventId) {
                if (ReadyEvents.empty() || *ReadyEvents.rbegin() <= lockedHistoryEventId) {
                    ReadyEvents.clear();
                } else {
                    ReadyEvents.erase(ReadyEvents.begin(), ReadyEvents.upper_bound(lockedHistoryEventId));
                }
                LockedHistoryEventId = lockedHistoryEventId;
            }
            InsertedEventsCount = Max(InsertedEventsCount, insertedEventsCount);
            LastRefreshInstant = Max(LastRefreshInstant, lastRefreshInstant);
        }
        {
            const TSet<TString> deps = GetDependencesAfter();
            for (auto&& i : deps) {
                SendGlobalMessage<TAfterCacheInvalidateNotification>(i, HistoryManager->GetCurrentInstant(), HistoryManager->GetTableName());
            }
        }
        return true;
    }
};

template <class TObjectContainer, class THistoryManager = TDBEntitiesHistoryManager<TObjectContainer>>
class TDBEntitiesCache: public TDBDirectOperator<TDBDirectOperatorBase<TDBCacheWithHistoryOwner<THistoryManager, TObjectContainer, typename TObjectContainer::TId>>> {
public:
    using TEntity = TObjectContainer;
private:
    using TBase = TDBDirectOperator<TDBDirectOperatorBase<TDBCacheWithHistoryOwner<THistoryManager, TObjectContainer, typename TObjectContainer::TId>>>;
    using TIdRef = typename TBase::TIdRef;
    using TId = typename TObjectContainer::TId;
protected:
    using TBase::HistoryCacheDatabase;
    using TBase::HistoryManager;
    using TBase::Objects;
    using TBase::MutexCachedObjects;

    virtual bool IsRemoveEvent(const EObjectHistoryAction action) const {
        return action == EObjectHistoryAction::Remove;
    }

    virtual TObjectContainer PrepareForUsage(const TObjectContainer& evHistory) const {
        return evHistory;
    }

    virtual bool ParsingStrictValidation() const {
        return false;
    }

    virtual bool DoRebuildCacheUnsafe() const override {
        auto transaction = HistoryCacheDatabase->CreateTransaction(true);
        NStorage::TObjectRecordsSet<TObjectContainer, TNull> objects;
        if (ParsingStrictValidation()) {
            objects.SetParsingFailPolicy(MakeAtomicShared<NStorage::TPanicOnFailPolicy>(TObjectContainer::GetTableName() + " parsing"));
        }
        {
            TSRSelect select(TObjectContainer::GetTableName(), &objects);
            if (!transaction->ExecRequest(select)->IsSucceed()) {
                ERROR_LOG << "Cannot refresh data for objects manager for " << TObjectContainer::GetTableName() << Endl;
                return false;
            }
        }

        for (auto&& i : objects) {
            Objects.emplace(i.GetInternalId(), PrepareForUsage(i));
        }

        return true;
    }

    virtual TIdRef GetEventObjectId(const TObjectEvent<TObjectContainer>& ev) const override {
        return ev.GetInternalId();
    }

    virtual void AcceptHistoryEventUnsafe(const TObjectEvent<TObjectContainer>& ev) const override final {
        if (IsRemoveEvent(ev.GetHistoryAction())) {
            DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
            Objects.erase(ev.GetInternalId());
        } else {
            auto& object = (Objects[ev.GetInternalId()] = PrepareForUsage(ev));
            DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
        }
    }

public:
    TDBEntitiesCache(const IHistoryContext& context, const THistoryConfig& historyConfig)
        : TBase(context.GetDatabase(), historyConfig) {
    }

    TDBEntitiesCache(THolder<IHistoryContext>&& context, const THistoryConfig& historyConfig)
        : TBase(context->GetDatabase(), historyConfig) {
    }

    TDBEntitiesCache(NStorage::IDatabase::TPtr db, const THistoryConfig& historyConfig)
        : TBase(db, historyConfig) {
    }

    TMaybe<TObjectContainer> GetCustomObject(const TId& objectId, const TInstant lastInstant = TInstant::Zero()) const {
        if (!TBase::RefreshCache(lastInstant)) {
            return Nothing();
        }
        TReadGuard rg(MutexCachedObjects);
        auto it = Objects.find(objectId);
        if (it == Objects.end()) {
            return Nothing();
        }
        return it->second;
    }

    template <class T>
    TAtomicSharedPtr<T> GetCustomObject(const TId& objectId, const TInstant lastInstant = TInstant::Zero()) const {
        auto container = GetCustomObject(objectId, lastInstant);
        if (!container) {
            return nullptr;
        }
        return container->template GetPtrAs<T>();
    }

    TSet<TString> GetObjectIds() const {
        TReadGuard rg(MutexCachedObjects);
        return MakeSet(NContainer::Keys(Objects));
    }

protected:
    virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TObjectContainer>& /*ev*/) const {
        return;
    }

    virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TObjectContainer>& /*ev*/, TObjectContainer& /*object*/) const {
        return;
    }
};

template<class TEntity, class TObjectId = TString>
class TAutoActualizingSnapshot : public IHistoryCallback<TObjectEvent<TEntity>, TObjectId>, public IMessageProcessor {
    using TBase = IHistoryCallback<TObjectEvent<TEntity>, TObjectId>;
    using THistoryReader = TAtomicSharedPtr<TCallbackSequentialTableImpl<TObjectEvent<TEntity>, TObjectId>>;
protected:
    using TBase::MutexCachedObjects;
    using TBase::Objects;

protected:
    THistoryReader HistoryReader;

protected:
    virtual bool DoRebuildCacheUnsafe() const = 0;

public:
    TAutoActualizingSnapshot(THistoryReader historyReader)
        : HistoryReader(historyReader)
    {
        HistoryReader->RegisterCallback(this);
        RegisterGlobalMessageProcessor(this);
    }

    virtual ~TAutoActualizingSnapshot() {
        UnregisterGlobalMessageProcessor(this);
    }

    virtual bool RefreshCache(const TInstant reqActuality, const bool doActualizeHistory = true) const override {
        Y_UNUSED(doActualizeHistory);
        return HistoryReader->Update(reqActuality);
    }

    bool RebuildCache() {
        TWriteGuard wg(MutexCachedObjects);
        return DoRebuildCacheUnsafe();
    }

    virtual bool Process(IMessage* message) override {
        const NFrontend::TCacheRefreshMessage* dropCache = dynamic_cast<const NFrontend::TCacheRefreshMessage*>(message);
        if (dropCache) {
            if (dropCache->GetComponents().empty() || dropCache->GetComponents().contains(HistoryReader->GetTableName())) {
                INFO_LOG << "Refresh Cache for " << HistoryReader->GetTableName() << Endl;
                HistoryReader->Update(Now());
                INFO_LOG << "Refresh Cache for " << HistoryReader->GetTableName() << " OK" << Endl;
            }
            return true;
        }
        return false;
    }

    virtual TString Name() const override {
        return "callback_cache_history_" + ToString(Now().GetValue()) + "_" + ToString(RandomNumber<ui64>());
    }

};
