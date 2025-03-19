#pragma once
#include <kernel/common_server/api/history/db_entities.h>
#include "link.h"

class TLinkManagerConfig {
private:
    CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
public:
    void Init(const TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        auto it = children.find("HistoryConfig");
        if (it != children.end()) {
            HistoryConfig.Init(it->second);
        }
    }
    void ToString(IOutputStream& os) const {
        os << "<HistoryConfig>" << Endl;
        HistoryConfig.ToString(os);
        os << "</HistoryConfig>" << Endl;
    }
};

template <class TDBLinkExternal>
class TLinksManager: public TDBEntitiesCache<TDBLinkExternal> {
private:
    using TDBLink = TDBLinkExternal;
    using TBase = TDBEntitiesCache<TDBLink>;
    using TOwnerId = typename TDBLinkExternal::TOwnerId;
    using TSlaveId = typename TDBLinkExternal::TSlaveId;
    class TIndexByOwnerIdPolicy {
    public:
        using TKey = typename TDBLinkExternal::TOwnerId;
        using TObject = TDBLink;
        static const TOwnerId& GetKey(const TDBLink& object) {
            return object.GetOwnerId();
        }
        static typename TDBLink::TId GetUniqueId(const TDBLink& object) {
            return object.GetLinkId();
        }
    };
public:
    using TIndexByOwnerId = TObjectByKeyIndex<TIndexByOwnerIdPolicy>;
protected:
    mutable TIndexByOwnerId IndexByOwnerId;
    virtual bool DoRebuildCacheUnsafe() const override {
        if (!TBase::DoRebuildCacheUnsafe()) {
            return false;
        }
        IndexByOwnerId.Initialize(TBase::Objects);
        return true;
    }

    virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBLink>& ev) const override {
        TBase::DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
        IndexByOwnerId.Remove(ev);
    }

    virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBLink>& ev, TDBLink& object) const override {
        TBase::DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
        IndexByOwnerId.Upsert(ev);
    }
public:
    TSet<TSlaveId> GetSlaves(const TOwnerId& ownerId) const {
        TVector<TDBLink> slaves;
        {
            TReadGuard rg(TBase::MutexCachedObjects);
            slaves = IndexByOwnerId.GetObjectsByKey(ownerId);
        }
        TSet<TSlaveId> result;
        for (auto&& i : slaves) {
            result.emplace(i.GetSlaveId());
        }
        return result;
    }

    TIndexByOwnerId DetachIndexByOwnerId() const {
        TReadGuard rg(TBase::MutexCachedObjects);
        return IndexByOwnerId;
    }
    bool Link(const TOwnerId& ownerId, const TSlaveId& slaveId, const TString& userId, NCS::TEntitySession& session) const {
        NStorage::TTableRecord trUpdate;
        trUpdate.Set("owner_id", ownerId).Set("slave_id", slaveId);
        return TBase::UpsertRecordNative(trUpdate, {"owner_id", "slave_id"}, userId, session);
    }

    bool RemoveByOwnerId(const TSet<TOwnerId>& ownerIds, const TString& userId, NCS::TEntitySession& session) const {
        if (ownerIds.empty()) {
            return true;
        }
        return TBase::RemoveRecords(TSRCondition().Init<TSRBinary>("owner_id", ownerIds), userId, session, nullptr);
    }

    bool AddLinks(const TOwnerId& ownerId, const TSet<TSlaveId>& slaveIds, const TString& userId, NCS::TEntitySession& session) const {
        for (auto&& i : slaveIds) {
            if (!Link(ownerId, i, userId, session)) {
                return false;
            }
        }
        return true;
    }

    TLinksManager(const IHistoryContext& hContext, const TLinkManagerConfig& config)
        : TBase(hContext, config.GetHistoryConfig()) {

    }

    TLinksManager(const NStorage::IDatabase::TPtr db, const THistoryConfig& hConfig)
        : TBase(db, hConfig) {

    }
};

