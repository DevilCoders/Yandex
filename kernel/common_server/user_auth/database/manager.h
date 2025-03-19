#pragma once
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/abstract/frontend.h>
#include "object.h"
#include "config.h"

class TDBAuthUsersManager: public TDBEntitiesCache<TDBAuthUserLink>, public IAuthUsersManager {
private:
    using TBase = TDBEntitiesCache<TDBAuthUserLink>;
    const TDBAuthUsersManagerConfig Config;
    TSRCondition BuildAuthIdRequest(const TAuthUserLinkId& authId) const;
protected:
    virtual bool DoAuthUserIdToInternalUserId(const TAuthUserLinkId& authId, TString& systemUserId) const override;
    class TIndexByAuthIdPolicy {
    private:
        CS_ACCESS(TIndexByAuthIdPolicy, bool, UseAuthModuleId, false);
    public:
        using TKey = TString;
        using TObject = TDBAuthUserLink;
        TString GetKey(const TDBAuthUserLink& object) {
            return object.GetAuthIdString(UseAuthModuleId);
        }
        TDBAuthUserLink::TId GetUniqueId(const TDBAuthUserLink& object) {
            return object.GetLinkId();
        }
    };
public:
    using TIndexByAuthId = TObjectByKeyIndex<TIndexByAuthIdPolicy>;
protected:
    mutable TIndexByAuthId IndexByAuthId;
    virtual bool DoRebuildCacheUnsafe() const override {
        if (!TBase::DoRebuildCacheUnsafe()) {
            return false;
        }
        IndexByAuthId.Initialize(TBase::Objects);
        return true;
    }

    virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBAuthUserLink>& ev) const override {
        TBase::DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
        IndexByAuthId.Remove(ev);
    }

    virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBAuthUserLink>& ev, TDBAuthUserLink& object) const override {
        TBase::DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
        IndexByAuthId.Upsert(ev);
    }
public:
    virtual bool StartManager() override {
        return TBase::Start();
    }
    virtual bool StopManager() override {
        return TBase::Stop();
    }
    virtual bool Upsert(const TAuthUserLink& userData, const TString& userId, bool* isUpdate) const override;
    virtual bool Remove(const TVector<TAuthUserLinkId>& authUserIds, const TString& userId) const override;
    virtual bool Remove(const TVector<ui32>& linkIds, const TString& userId) const override;
    virtual bool Restore(const TAuthUserLinkId& authUserId, TMaybe<TAuthUserLink>& data) const override;
    virtual bool Restore(const TString& systemUserId, TVector<TAuthUserLink>& data) const override;
    virtual bool RestoreAll(TVector<TAuthUserLink>& data) const override;

    TDBAuthUsersManager(THolder<IHistoryContext>&& hContext, const TDBAuthUsersManagerConfig& rmConfig, const IBaseServer& server)
        : TBase(std::move(hContext), rmConfig.GetHistoryConfig())
        , IAuthUsersManager(server)
        , Config(rmConfig)
    {
        IndexByAuthId.MutablePolicy().SetUseAuthModuleId(rmConfig.GetUseAuthModuleId());
    }
};
