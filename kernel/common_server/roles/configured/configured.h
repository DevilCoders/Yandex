#pragma once
#include <kernel/common_server/util/algorithm/container.h>
#include <kernel/common_server/roles/abstract/manager.h>
#include <library/cpp/yconf/conf.h>

class TConfiguredRolesManagerConfig: public IRolesManagerConfig {
private:
    static TFactory::TRegistrator<TConfiguredRolesManagerConfig> Registrator;

    class TItemsConstructor : public IHierarchyRolesConstructor {
    private:
        const TConfiguredRolesManagerConfig& Config;
    public:
        TItemsConstructor(const TConfiguredRolesManagerConfig& config)
            : Config(config)
        {}
    protected:
        virtual TVector<TString> GetSlaveRoles(const TString& roleId) const {
            return Config.GetRolesByRole(roleId);
        }

        virtual TVector<TString> GetSlaveItems(const TString& roleId) const {
            return Config.GetItemsByRole(roleId);
        }
    };


    const TVector<TString> GetItemsByRole(const TString& roleId) const {
        TReadGuard g(Mutex);
        auto it = InfoByRole.find(roleId);
        return (it == InfoByRole.end()) ? Default<TVector<TString>>() : it->second.first;
    }

    const TVector<TString> GetRolesByRole(const TString& roleId) const {
        TReadGuard g(Mutex);
        auto it = InfoByRole.find(roleId);
        return (it == InfoByRole.end()) ? Default<TVector<TString>>() : it->second.second;
    }

protected:
    mutable TMap<TString, TItemPermissionContainer> Items;
    mutable TMap<TString, std::pair<TVector<TString>, TVector<TString>>> InfoByRole;
    TRWMutex Mutex;

public:
    static TString GetTypeName() {
        return "configured";
    }

    virtual TString GetClassName() const override {
        return GetTypeName();
    }

    IHierarchyRolesConstructor::TPtr GetItemsConstructor() const {
        return MakeAtomicShared<TItemsConstructor>(*this);
    }

    TItemPermissionContainer GetItem(const TString& itemId) const;

    virtual void DoToString(IOutputStream& os) const override;
    virtual void DoInit(const TYandexConfig::Section* section) override;
    virtual THolder<IRolesManager> BuildManager(const IBaseServer& /*server*/) const override;
};

template <class TRolesConfig = TConfiguredRolesManagerConfig>
class TConfiguredRolesManager: public IHierarchyRolesManager {
protected:
    const TRolesConfig& Config;

protected:
    virtual IHierarchyRolesConstructor::TPtr GetItemsConstructor() const override {
        return Config.GetItemsConstructor();
    }

    virtual TVector<TItemPermissionContainer> RestoreItems(const TSet<TString>& itemIds) const override {
        TVector<TItemPermissionContainer> result;
        for (auto&& i : itemIds) {
            auto item = Config.GetItem(i);
            if (!item) {
                continue;
            }
            result.emplace_back(item);
        }
        return result;
    }

public:
    static TString GetTypeName() {
        return "configured";
    }

    TConfiguredRolesManager(const TRolesConfig& config)
        : Config(config)
    {
    }
};
