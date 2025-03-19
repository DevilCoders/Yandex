#pragma once
#include "action.h"
#include "role.h"
#include "config.h"
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/api/links/manager.h>

class TDBItemsManager: public TDBEntitiesManager<TDBItemPermissions> {
private:
    using TBase = TDBEntitiesManager<TDBItemPermissions>;
public:
    TDBItemsManager(const IHistoryContext& hContext, const TDBItemsManagerConfig& config)
        : TBase(hContext, config.GetManagerConfig()) {

    }
};

class TDBRolesManager: public TDBEntitiesManager<TDBRole> {
private:
    using TBase = TDBEntitiesManager<TDBRole>;
protected:
    mutable TMap<ui32, TString> IndexByRoleId;
    virtual bool DoRebuildCacheUnsafe() const override {
        if (!TBase::DoRebuildCacheUnsafe()) {
            return false;
        }
        for (auto&& i : TBase::Objects) {
            IndexByRoleId[i.second.GetRoleId()] = i.second.GetRoleName();
        }
        return true;
    }

    virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBRole>& ev) const override {
        TBase::DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
        IndexByRoleId.erase(ev.GetRoleId());
    }

    virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBRole>& ev, TDBRole& object) const override {
        TBase::DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
        IndexByRoleId.emplace(ev.GetRoleId(), ev.GetRoleName());
    }
public:
    TSet<TString> GetRoleNameByIds(const TSet<ui32>& roleIds) const {
        TSet<TString> result;
        TReadGuard rg(MutexCachedObjects);
        for (auto&& r : roleIds) {
            auto it = IndexByRoleId.find(r);
            if (it == IndexByRoleId.end()) {
                continue;
            }
            result.emplace(it->second);
        }
        return result;
    }

    TDBRolesManager(const IHistoryContext& hContext, const TDBRolesManagerConfig& config)
        : TBase(hContext, config.GetManagerConfig()) {

    }

    bool RestoreByRoleName(const TSet<TString>& roleNames, TVector<TDBRole>& objects, NCS::TEntitySession& session) const {
        if (roleNames.empty()) {
            objects.clear();
            return true;
        }
        return RestoreObjectsBySRCondition(objects, TSRCondition().Init<TSRBinary>("role_name", roleNames), session);
    }

    bool UpsertByRoleName(const TUserRoleContainer& obj, const TString& userId, NCS::TEntitySession& session, bool* isUpdate) const {
        TDBRole dbObj(obj);
        return UpsertObject(dbObj, userId, session, isUpdate);
    }

    bool RemoveByRoleName(const TSet<TString>& roleNames, const TString& userId, NCS::TEntitySession& session) const {
        if (roleNames.empty()) {
            return true;
        }
        return RemoveRecords(TSRCondition().Init<TSRBinary>("role_name", roleNames), userId, session);
    }
};

class TDBLinkRoleItem: public TDBLink<TString, TString> {
public:
    static TString GetTableName() {
        return "links_role_item";
    }

    static TString GetHistoryTableName() {
        return "links_role_item_history";
    }
};

class TDBLinkRoleRole: public TDBLink<TString, TString> {
public:
    static TString GetTableName() {
        return "links_role_role";
    }

    static TString GetHistoryTableName() {
        return "links_role_role_history";
    }
};

class TRoleRoleLinksManager: public TLinksManager<TDBLinkRoleRole> {
private:
    using TBase = TLinksManager<TDBLinkRoleRole>;
public:
    TRoleRoleLinksManager(const IHistoryContext& hContext, const TLinkManagerConfig& config)
        : TBase(hContext, config) {

    }
};

class TRoleItemLinksManager: public TLinksManager<TDBLinkRoleItem> {
private:
    using TBase = TLinksManager<TDBLinkRoleItem>;
public:
    TRoleItemLinksManager(const IHistoryContext& hContext, const TLinkManagerConfig& config)
        : TBase(hContext, config) {

    }
};

class TDBFullRolesManager: public IHierarchyRolesManager {
private:
    using TBase = IHierarchyRolesManager;
    THolder<THistoryContext> HistoryContext;
    THolder<TDBItemsManager> ItemsManager;
    THolder<TDBRolesManager> RolesManager;
    THolder<TRoleRoleLinksManager> RoleRoleLinksManager;
    THolder<TRoleItemLinksManager> RoleItemLinksManager;
protected:
    class THierarchyRolesConstructor: public IHierarchyRolesConstructor {
    private:
        TRoleRoleLinksManager::TIndexByOwnerId RoleRoleIndexByOwnerId;
        TRoleItemLinksManager::TIndexByOwnerId RoleItemIndexByOwnerId;
    public:
        THierarchyRolesConstructor(TRoleRoleLinksManager::TIndexByOwnerId&& rrIndex, TRoleItemLinksManager::TIndexByOwnerId&& riIndex)
            : RoleRoleIndexByOwnerId(std::move(rrIndex))
            , RoleItemIndexByOwnerId(std::move(riIndex))
        {

        }

        virtual TVector<TString> GetSlaveRoles(const TString& roleId) const override {
            auto links = RoleRoleIndexByOwnerId.GetObjectsByKey(roleId);
            TVector<TString> result;
            for (auto&& i : links) {
                result.emplace_back(i.GetSlaveId());
            }
            return result;
        }
        virtual TVector<TString> GetSlaveItems(const TString& roleId) const override {
            auto links = RoleItemIndexByOwnerId.GetObjectsByKey(roleId);
            TVector<TString> result;
            for (auto&& i : links) {
                result.emplace_back(i.GetSlaveId());
            }
            return result;
        }
    };

    virtual IHierarchyRolesConstructor::TPtr GetItemsConstructor() const override {
        return MakeAtomicShared<THierarchyRolesConstructor>(RoleRoleLinksManager->DetachIndexByOwnerId(), RoleItemLinksManager->DetachIndexByOwnerId());
    }
    virtual TVector<TItemPermissionContainer> RestoreItems(const TSet<TString>& itemIds) const override {
        TVector<TItemPermissionContainer> result;
        TVector<TDBItemPermissions> dbResult;
        CHECK_WITH_LOG(ItemsManager->GetCustomObjects(dbResult, itemIds));
        for (auto&& i : dbResult) {
            result.emplace_back(i);
        }
        return result;
    }

    virtual bool DoStart() override;
    virtual bool DoStop() override;
public:
    virtual bool UpsertRole(const TUserRoleInfo& role, const TString& userId, bool* isUpdate = nullptr) const override {
        auto session = RolesManager->BuildNativeSession(false);
        if (!RolesManager->UpsertByRoleName(role, userId, session, isUpdate)) {
            return false;
        }
        if (!RoleRoleLinksManager->RemoveByOwnerId({ role.GetRoleName() }, userId, session)) {
            return false;
        }
        if (!RoleItemLinksManager->RemoveByOwnerId({ role.GetRoleName() }, userId, session)) {
            return false;
        }
        if (!RoleRoleLinksManager->AddLinks(role.GetRoleName(), role.GetRoleIds(), userId, session)) {
            return false;
        }
        if (!RoleItemLinksManager->AddLinks(role.GetRoleName(), role.GetItemIds(), userId, session)) {
            return false;
        }
        return session.Commit();
    }
    virtual bool RemoveRoles(const TSet<TString>& roleNames, const TString& userId) const override {
        auto session = RolesManager->BuildNativeSession(false);
        if (!RoleRoleLinksManager->RemoveByOwnerId(roleNames, userId, session)) {
            return false;
        }
        if (!RoleItemLinksManager->RemoveByOwnerId(roleNames, userId, session)) {
            return false;
        }
        if (!RolesManager->RemoveByRoleName(roleNames, userId, session)) {
            return false;
        }
        return session.Commit();
    }
    virtual bool GetRoles(TVector<TUserRoleInfo>& result) const override {
        TVector<TDBRole> dbRoles;
        if (!RolesManager->GetAllObjects(dbRoles)) {
            return false;
        }
        TVector<TDBLinkRoleRole> dbRoleRoleLinks;
        if (!RoleRoleLinksManager->GetAllObjects(dbRoleRoleLinks)) {
            return false;
        }
        TVector<TDBLinkRoleItem> dbRoleItemLinks;
        if (!RoleItemLinksManager->GetAllObjects(dbRoleItemLinks)) {
            return false;
        }
        {
            TMap<TString, TUserRoleInfo> resultMap;
            for (auto&& i : dbRoles) {
                resultMap.emplace(i.GetRoleName(), TUserRoleInfo(i));
            }
            for (auto&& i : dbRoleRoleLinks) {
                auto it = resultMap.find(i.GetOwnerId());
                if (it == resultMap.end()) {
                    continue;
                }
                it->second.MutableRoleIds().emplace(i.GetSlaveId());
            }
            for (auto&& i : dbRoleItemLinks) {
                auto it = resultMap.find(i.GetOwnerId());
                if (it == resultMap.end()) {
                    continue;
                }
                it->second.MutableItemIds().emplace(i.GetSlaveId());
            }
            TVector<TUserRoleInfo> resultLocal;
            for (auto&& [id, value] : resultMap) {
                resultLocal.emplace_back(std::move(value));
            }
            std::swap(resultLocal, result);
        }
        return true;
    }

    virtual bool UpsertItem(const TItemPermissionContainer& item, const TString& userId, bool* isUpdate = nullptr) const override {
        TDBItemPermissions dbObject(item);
        auto session = ItemsManager->BuildNativeSession(false);
        return ItemsManager->UpsertObject(dbObject, userId, session, isUpdate) && session.Commit();
    }
    virtual bool RemoveItems(const TSet<TString>& itemNames, const TString& userId) const override {
        auto session = ItemsManager->BuildNativeSession(false);
        return ItemsManager->RemoveObjects(itemNames, userId, session) && session.Commit();
    }
    virtual bool GetItems(TVector<TItemPermissionContainer>& result) const override {
        TVector<TDBItemPermissions> items;
        if (!ItemsManager->GetAllObjects(items)) {
            return false;
        }
        for (auto&& i : items) {
            result.emplace_back(std::move(i));
        }
        return true;
    }

    virtual TSet<TString> RestoreRoleNames(const TSet<ui32>& roleIds) const override {
        return RolesManager->GetRoleNameByIds(roleIds);
    }

    const TDBRolesManager& GetRolesManager() const {
        return *RolesManager;
    }

    TDBFullRolesManager(const IBaseServer& server, const TDBFullRolesManagerConfig& config)
        : HistoryContext(MakeHolder<THistoryContext>(server.GetDatabase(config.GetDBName())))
        , ItemsManager(MakeHolder<TDBItemsManager>(*HistoryContext, config.GetItemsManagerConfig()))
        , RolesManager(MakeHolder<TDBRolesManager>(*HistoryContext, config.GetRolesManagerConfig()))
        , RoleRoleLinksManager(MakeHolder<TRoleRoleLinksManager>(*HistoryContext, config.GetRoleRoleLinksManagerConfig()))
        , RoleItemLinksManager(MakeHolder<TRoleItemLinksManager>(*HistoryContext, config.GetRoleItemLinksManagerConfig()))
    {

    }
};
