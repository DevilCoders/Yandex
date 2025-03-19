#pragma once
#include "role.h"
#include "item.h"

class IRolesManager: public IAutoActualization {
public:
    IRolesManager()
        : IAutoActualization("RolesManager")
    {}

    using TPtr = TAtomicSharedPtr<IRolesManager>;

    virtual bool Refresh() override {
        return true;
    }

    virtual ~IRolesManager() = default;
    virtual TVector<TItemPermissionContainer> GetRoleItems(const TSet<TString>& roleIds) const = 0;
    virtual TSet<TString> RestoreRoleNames(const TSet<ui32>& /*roleIds*/) const {
        S_FAIL_LOG << "Not implemented: incorrect server configuration: db-link user-role and non-db-roles_manager" << Endl;
        return Default<TSet<TString>>();
    }
    virtual bool UpsertRole(const TUserRoleInfo& role, const TString& userId, bool* isUpdate = nullptr) const;
    virtual bool RemoveRoles(const TSet<TString>& roleNames, const TString& userId) const;
    virtual bool GetRoles(TVector<TUserRoleInfo>& roles) const;

    virtual bool UpsertItem(const TItemPermissionContainer& item, const TString& userId, bool* isUpdate = nullptr) const;
    virtual bool RemoveItems(const TSet<TString>& itemNames, const TString& userId) const;
    virtual bool GetItems(TVector<TItemPermissionContainer>& items) const;

    virtual TSet<TString> GetItemIds() const;
    virtual TSet<TString> GetRoleNames() const;
};

class IHierarchyRolesConstructor {
protected:
    virtual TVector<TString> GetSlaveRoles(const TString& roleId) const = 0;
    virtual TVector<TString> GetSlaveItems(const TString& roleId) const = 0;
public:
    virtual ~IHierarchyRolesConstructor() = default;
    using TPtr = TAtomicSharedPtr<IHierarchyRolesConstructor>;
    TSet<TString> GetItems(const TSet<TString>& roleIds) const {
        TSet<TString> currentRoles = roleIds;
        TSet<TString> readyRoles = roleIds;
        TSet<TString> readyItems;
        while (currentRoles.size()) {
            TSet<TString> nextRolesSet;
            for (auto&& i : currentRoles) {
                const TVector<TString> slaveRoles = GetSlaveRoles(i);
                const TVector<TString> slaveItems = GetSlaveItems(i);
                readyItems.insert(slaveItems.begin(), slaveItems.end());
                for (auto&& l : slaveRoles) {
                    if (!readyRoles.emplace(l).second) {
                        continue;
                    }
                    nextRolesSet.emplace(l);
                }
            }
            std::swap(currentRoles, nextRolesSet);
        }
        return readyItems;
    }
};

class IHierarchyRolesManager: public IRolesManager {
protected:
    virtual IHierarchyRolesConstructor::TPtr GetItemsConstructor() const = 0;
    virtual TVector<TItemPermissionContainer> RestoreItems(const TSet<TString>& itemIds) const = 0;
public:
    virtual TVector<TItemPermissionContainer> GetRoleItems(const TSet<TString>& roleIds) const override final {
        IHierarchyRolesConstructor::TPtr constructor = GetItemsConstructor();
        CHECK_WITH_LOG(!!constructor);
        TSet<TString> items = constructor->GetItems(roleIds);
        return RestoreItems(items);
    }
};

class IRolesManagerConfig {
protected:
    virtual void DoInit(const TYandexConfig::Section* section) = 0;
    virtual void DoToString(IOutputStream& os) const = 0;
public:
    using TPtr = TAtomicSharedPtr<IRolesManagerConfig>;
    using TFactory = NObjectFactory::TObjectFactory<IRolesManagerConfig, TString>;
    virtual ~IRolesManagerConfig() = default;
    virtual THolder<IRolesManager> BuildManager(const IBaseServer& server) const = 0;
    virtual TString GetClassName() const = 0;

    void Init(const TYandexConfig::Section* section) {
        DoInit(section);
    }

    void ToString(IOutputStream& os) const {
        DoToString(os);
    }
};

class TRolesManagerConfig: public TBaseInterfaceContainer<IRolesManagerConfig> {
public:
};
