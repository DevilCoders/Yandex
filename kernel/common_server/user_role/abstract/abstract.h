#pragma once
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/roles/abstract/item.h>
#include <kernel/common_server/roles/abstract/manager.h>
#include <kernel/common_server/util/map_processing.h>
#include <util/string/split.h>


namespace NCSPermissionsUtil {
    template <class TDst, class TSrc>
    bool Merge(TDst& dst, TSrc&& src);
}


class IUserPermissions {
private:
    CSA_DEFAULT(IUserPermissions, TString, UserId);
    CSA_DEFAULT(IUserPermissions, TString, OriginatorId);
protected:
    TVector<TItemPermissionContainer> UserAbilities;
    virtual bool IsTransparent() const {
        return false;
    }
public:
    IUserPermissions(const TString& userId)
        : UserId(userId)
    {

    }

    template <class T, class EAction>
    TSet<TString> GetObjectNames(const EAction action) const {
        TSet<TString> result;
        for (auto&& ability : UserAbilities) {
            auto item = ability.GetPtrAs<T>();
            if (!item || !item->GetActions().contains(action)) {
                continue;
            }
            const auto& names = item->GetObjectNames();
            result.insert(names.begin(), names.end());
        }
        return result;
    }

    template <class T>
    TSet<TString> GetObjectNames() const {
        TSet<TString> result;
        for (auto&& ability : UserAbilities) {
            auto item = ability.GetPtrAs<T>();
            if (!item) {
                continue;
            }
            const auto& names = item->GetObjectNames();
            result.insert(names.begin(), names.end());
        }
        return result;
    }

    template <class T, class... TArgs>
    bool Check(TArgs... args) const {
        if (ICSOperator::GetServer().IsNoPermissionsMode()) {
            return true;
        }
        if (IsTransparent()) {
            return true;
        }
        for (auto&& ability : UserAbilities) {
            auto item = ability.GetPtrAs<T>();
            if (item && item->GetEnabled() && item->Check(args...)) {
                return true;
            }
        }
        return false;
    }

    template <class T>
    bool Has() const {
        if (ICSOperator::GetServer().IsNoPermissionsMode()) {
            return true;
        }
        if (IsTransparent()) {
            return true;
        }
        for (auto&& ability : UserAbilities) {
            auto item = ability.GetPtrAs<T>();
            if (item) {
                return true;
            }
        }
        return false;
    }

    using TPtr = TAtomicSharedPtr<IUserPermissions>;
    virtual ~IUserPermissions() = default;
    virtual void AddAbilities(const TVector<TItemPermissionContainer>& abilities);
    virtual TItemPermissionContainer GetItemPermissions(const TString& item) const;

    template <class T>
    TVector<TAtomicSharedPtr<T>> GetItemPermissionsByClass(const TString& className = Default<TString>()) const {
        TVector<TAtomicSharedPtr<T>> result;
        for (auto&& i : UserAbilities) {
            auto item = i.GetPtrAs<T>();
            if (item && item->GetEnabled() && (!className || className == item->GetClassName())) {
                result.emplace_back(item);
            }
        }
        return result;
    }

    template <class T>
    TVector<TItemPermissionContainer> GetItemPermissionContainersByClass(const TString& className = Default<TString>()) const {
        TVector<TItemPermissionContainer> result;
        for (auto&& i : UserAbilities) {
            auto item = i.GetPtrAs<T>();
            if (item && item->GetEnabled() && (!className || className == item->GetClassName())) {
                result.emplace_back(i);
            }
        }
        return result;
    }

    template <class T, class TResult = T>
    auto GetPermissionsMerged(const TString& className = Default<TString>(), TResult&& result = {}) const {
        auto permissions = GetItemPermissionsByClass<T>(className);
        for (auto&& permission : permissions) {
            if (!NCSPermissionsUtil::Merge(result, *permission)) {
                result = {};
                break;
            }
        }
        return std::forward<TResult>(result);
    }

    template <class T, class TActions>
    auto GetPermissionsMergedByActions(const TActions actions, const TString& className = Default<TString>()) const {
        T result;
        result.SetActions(actions);
        for (auto&& permission : GetItemPermissionsByClass<T>(className)) {
            if (!permission->Check(result.GetActions())) {
                continue;
            }

            auto narrowedPermission = *permission;
            narrowedPermission.SetActions(result.GetActions());

            if (!NCSPermissionsUtil::Merge(result, narrowedPermission)) {
                result = {};
                break;
            }
        }
        return result;
    }

    template <class T>
    TAtomicSharedPtr<T> GetItemPermissionsAs(const TString& item) const {
        return GetItemPermissions(item).GetPtrAs<T>();
    }

    virtual void Init(const TYandexConfig::Section* /*section*/) {
        CHECK_WITH_LOG(false) << "Incorrect method" << Endl;
    }
    virtual void ToString(IOutputStream& /*os*/) const {
        CHECK_WITH_LOG(false) << "Incorrect method" << Endl;
    }
};

class TTransparentPermissions: public IUserPermissions {
private:
    using TBase = IUserPermissions;
protected:
    virtual bool IsTransparent() const override {
        return true;
    }
public:
    using TBase::TBase;
};

class TUserRolesCompiled {
private:
    CSA_DEFAULT(TUserRolesCompiled, TString, SystemUserId);
    CSA_DEFAULT(TUserRolesCompiled, TSet<TString>, RoleNames);
public:
    static NFrontend::TScheme GetScheme(const IBaseServer& server);
    static NFrontend::TScheme GetSearchScheme(const IBaseServer& server);
    bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    NJson::TJsonValue SerializeToJson() const;
    void Merge(const TUserRolesCompiled& other);
};

class IPermissionsManagerConfig;

class IPermissionsManager : public IAutoActualization {
    CSA_DEFAULT(IPermissionsManager, TString, DefaultUser);
public:
    class TGetPermissionsContext {
    public:
        TGetPermissionsContext(const TString& userId, const TString& authModuleName = Default<TString>());
        CSA_READONLY_DEF(TString, UserId);
        CSA_READONLY_DEF(TString, AuthModuleName);
    };

    class IUserInfoExtender {
    public:
        using TPtr = TAtomicSharedPtr<IUserInfoExtender>;
        struct TExtendedUserInfo {
            TSet<TString> UserIdAliases;
        };
        virtual ~IUserInfoExtender() = default;
        virtual bool FillExtendedInfo(TExtendedUserInfo& result, const TGetPermissionsContext& context) const = 0;
    };

protected:
    const IBaseServer& Server;
    IUserInfoExtender::TPtr UserInfoExtender;

public:
    IPermissionsManager(const IBaseServer& server, const IPermissionsManagerConfig& config);

    virtual bool Refresh() override {
        return true;
    }

    virtual bool Restore(const TString& systemUserId, TUserRolesCompiled& result) const = 0;
    virtual bool RestoreAll(TVector<TUserRolesCompiled>& result) const {
        Y_UNUSED(result);
        return false;
    };
    virtual bool Upsert(const TUserRolesCompiled& result, const TString& userId, bool* isUpdate) const = 0;

    virtual ~IPermissionsManager() = default;
    virtual IUserPermissions::TPtr GetPermissions(const TGetPermissionsContext& context) const {
        IUserInfoExtender::TExtendedUserInfo extInfo;
        if (!UserInfoExtender->FillExtendedInfo(extInfo, context)) {
            return nullptr;
        }
        TUserRolesCompiled compilation;
        for (const auto& a: extInfo.UserIdAliases) {
            TUserRolesCompiled c;
            if (!Restore(a, c)) {
                return nullptr;
            }
            compilation.Merge(c);
        }

        if (compilation.GetRoleNames().empty() && DefaultUser) {
            if (!Restore(DefaultUser, compilation)) {
                return nullptr;
            }
        }
        compilation.SetSystemUserId(context.GetUserId());
        return Server.BuildPermissionFromItems(Server.GetRolesManager().GetRoleItems(compilation.GetRoleNames()), context.GetUserId());
    }
    template <class T>
    TAtomicSharedPtr<T> GetPermissions(const TGetPermissionsContext& context) const {
        return DynamicPointerCast<T>(GetPermissions(context));
    }
};

class IPermissionsManagerConfig {
public:
    class IUserInfoExtenderConfig {
    public:
        virtual ~IUserInfoExtenderConfig() = default;
        virtual TAtomicSharedPtr<IPermissionsManager::IUserInfoExtender> ConstructUserInfoExtender(const IBaseServer& server) const = 0;
        virtual TString GetClassName() const = 0;
        void ToString(IOutputStream& os) const {
            DoToString(os);
        }
        void Init(const TYandexConfig::Section* section) {
            DoInit(section);
        }

    public:
        using TFactory = NObjectFactory::TParametrizedObjectFactory<IUserInfoExtenderConfig, TString>;
        using TPtr = TAtomicSharedPtr<IUserInfoExtenderConfig>;
    protected:
        virtual void DoToString(IOutputStream& os) const = 0;
        virtual void DoInit(const TYandexConfig::Section* section) = 0;
    };
    using TUserInfoExtenderConfig = TBaseInterfaceContainer<IUserInfoExtenderConfig>;

    CSA_READONLY_DEF(TString, DefaultUser);
    CSA_READONLY_DEF(TUserInfoExtenderConfig, UserInfoExtender);
protected:
    virtual void DoToString(IOutputStream& os) const = 0;
    virtual void DoInit(const TYandexConfig::Section* section) = 0;
public:
    using TPtr = TAtomicSharedPtr<IPermissionsManagerConfig>;
    virtual ~IPermissionsManagerConfig() = default;

    using TFactory = NObjectFactory::TObjectFactory<IPermissionsManagerConfig, TString>;

    virtual void Init(const TYandexConfig::Section* section) {
        const auto children = section->GetAllChildren();
        if (const auto uie = MapFindPtr(children, "UserInfoExtender")) {
            UserInfoExtender.Init(*uie);
        } else {
            UserInfoExtender.Init(nullptr);
        }
        DefaultUser = section->GetDirectives().Value<TString>("DefaultUser", DefaultUser);
        DoInit(section);
    }

    virtual void ToString(IOutputStream& os) const {
        os << "DefaultUser:" << DefaultUser << Endl;
        UserInfoExtender.ToString(os);
        DoToString(os);
    }

    virtual THolder<IPermissionsManager> BuildManager(const IBaseServer& server) const = 0;
    virtual TString GetClassName() const = 0;
};

class TPermissionsManagerConfig: public TBaseInterfaceContainer<IPermissionsManagerConfig> {
};


namespace NCSPermissionsUtil {
    template <class TDst, class TSrc>
    bool Merge(TDst& dst, TSrc&& src) {
        if constexpr (std::is_same_v<bool, decltype(dst.Merge(std::forward<TSrc>(src)))>) {
            return dst.Merge(std::forward<TSrc>(src));
        } else {
            dst.Merge(std::forward<TSrc>(src));
            return true;
        }
    }
}
