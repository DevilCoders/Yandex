#pragma once
#include "manager.h"
#include "sequential.h"
#include "event.h"
#include "common.h"

template <class TObjectContainer>
class TDBEntitiesHistoryManager: public TIndexedAbstractHistoryManager<TObjectContainer> {
public:
    using TIdRef = typename TIdTypeSelector<typename TObjectContainer::TId>::TIdRef;
    using TEntity = TObjectContainer;
private:
    using TBase = TIndexedAbstractHistoryManager<TObjectContainer>;
protected:
    class TEntityIndex: public IEventsIndex<TObjectEvent<TObjectContainer>, TIdRef> {
    private:
        using TBase = IEventsIndex<TObjectEvent<TObjectContainer>, TIdRef>;
    protected:
        virtual TMaybe<TIdRef> ExtractKey(TAtomicSharedPtr<TObjectEvent<TObjectContainer>> ev) const override {
            return ev->GetInternalId();
        }
    public:
        using TBase::TBase;
    };

    class TUserHistoryIndex: public IEventsIndex<TObjectEvent<TObjectContainer>> {
    private:
        using TBase = IEventsIndex<TObjectEvent<TObjectContainer>>;
    protected:
        virtual TMaybe<TStringBuf> ExtractKey(TAtomicSharedPtr<TObjectEvent<TObjectContainer>> ev) const override {
            return ev->GetHistoryUserId();
        }
    public:
        using TBase::TBase;
    };

    THolder<TEntityIndex> EntityIndex;
    THolder<TUserHistoryIndex> UserHistoryIndex;
public:
    using TBase::TBase;

    TMaybe<TObjectContainer> GetMostActualObject(const TIdRef id, const TInstant until, const bool include) const {
        auto eventPtr = GetIndexByInternalId().GetLastEventById(id, until, include);
        if (!eventPtr) {
            return Nothing();
        }
        return *eventPtr;
    }

    const TEntityIndex& GetIndexByInternalId() const {
        return *EntityIndex;
    }

    const TUserHistoryIndex& GetIndexByHistoryUserId() const {
        return *UserHistoryIndex;
    }

    TDBEntitiesHistoryManager(const IHistoryContext& context, const THistoryConfig& config)
        : TBase(context, TObjectContainer::GetTableName() + "_history", config)
        , EntityIndex(MakeHolder<TEntityIndex>(this, "entity_internal_id"))
        , UserHistoryIndex(MakeHolder<TUserHistoryIndex>(this, "history_user"))
    {
    }
};

template <class TObjectContainer>
class TDBRevisionedEntitiesHistoryManager: public TDBEntitiesHistoryManager<TObjectContainer> {
private:
    using TBase = TDBEntitiesHistoryManager<TObjectContainer>;
public:
    using TIdRef = typename TBase::TIdRef;
protected:
    class TRevisionIndex: public IEventsIndex<TObjectEvent<TObjectContainer>, ui32> {
    private:
        using TBase = IEventsIndex<TObjectEvent<TObjectContainer>, ui32>;
    protected:
        virtual TMaybe<ui32> ExtractKey(TAtomicSharedPtr<TObjectEvent<TObjectContainer>> ev) const override {
            return ev->GetRevisionMaybe();
        }
    public:
        using TBase::TBase;
    };

    THolder<TRevisionIndex> RevisionIndex;
public:
    using TBase::TBase;
    using TBase::GetTableName;

    TMaybe<TObjectContainer> GetRevisionedObject(const ui32 revision, const TIdRef id) const {
        auto ev = GetRevisionedEventWithId(revision, id);
        if (!ev) {
            return Nothing();
        }
        return *ev;
    }

    TAtomicSharedPtr<TObjectEvent<TObjectContainer>> GetRevisionedEventWithId(const ui32 revision, const TIdRef id) const {
        auto ev = GetRevisionedEvent(revision);
        if (!ev) {
            return nullptr;
        }
        if (ev->GetInternalId() != id) {
            return nullptr;
        }
        return ev;
    }

    TAtomicSharedPtr<TObjectEvent<TObjectContainer>> GetRevisionedEvent(const ui32 revision) const {
        TDeque<TAtomicSharedPtr<TObjectEvent<TObjectContainer>>> events;
        CHECK_WITH_LOG(RevisionIndex->GetEvents(revision, events, TInstant::Zero()));
        if (!events.size()) {
            return nullptr;
        }
        if (events.size() != 1) {
            TFLEventLog::Alert("incorrect events by revision")("revision", revision)("table_name", GetTableName());
            return nullptr;
        }
        return events.front();
    }

    TDBRevisionedEntitiesHistoryManager(const IHistoryContext& context, const THistoryConfig& config)
        : TBase(context, config)
        , RevisionIndex(MakeHolder<TRevisionIndex>(this, "entity_revision")) {
    }
};
