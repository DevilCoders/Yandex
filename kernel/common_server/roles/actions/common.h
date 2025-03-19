#pragma once
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/util/map_processing.h>
#include <util/string/split.h>
#include <kernel/common_server/roles/abstract/item.h>
#include <kernel/common_server/user_role/abstract/abstract.h>

class TDefaultUserPermissions: public IUserPermissions {
private:
    using TBase = IUserPermissions;
public:
    using TBase::TBase;
};

template <class EAction, class TActions>
class TAdministrativePermissionsImpl: public IItemPermissions {
public:
    using EObjectAction = EAction;
    using TObjectActions = TActions;

private:
    using TSelf = TAdministrativePermissionsImpl<EAction, TActions>;
    using TBase = IItemPermissions;

    CSA_DEFAULT(TSelf, TSet<EObjectAction>, Actions);

public:
    bool Check(const EObjectAction action) const {
        return GetActions().contains(action);
    }

    bool Check(const TObjectActions actions) const {
        return (GetActionsMask() & actions) == actions;
    }

    bool Check(const TSet<EObjectAction>& actions) const {
        return std::includes(Actions.begin(), Actions.end(), actions.begin(), actions.end());
    }

    TSelf& SetActions(const EObjectAction action) noexcept {
        Actions = TSet<EObjectAction>{ action };
        return *this;
    }

    TSelf& SetActions(const TObjectActions actions) noexcept {
        Actions.clear();
        for (auto&& action : GetEnumAllValues<EObjectAction>()) {
            const auto actionValue = (TObjectActions)action;
            if ((actions & actionValue) == actionValue) {
                Actions.insert(action);
            }
        }
        return *this;
    }

    TObjectActions GetActionsMask() const {
        TObjectActions result = 0;
        for (auto&& i : Actions) {
            result |= (TObjectActions)i;
        }
        return result;
    }

    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme result = TBase::DoGetScheme(server);
        result.Add<TFSVariants>("actions").InitVariants<EObjectAction>().SetMultiSelect(true);
        return result;
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        TJsonProcessor::WriteContainerArrayStrings(result, "actions", Actions);
        return result;
    }

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        if (!TBase::DeserializeFromJson(info)) {
            return false;
        }
        return TJsonProcessor::ReadContainer(info, "actions", Actions);
    }
};

namespace NCommonAdministrative {
    enum class EObjectAction: ui64 {
        Add = 1 /* "add" */,
        Modify = 1 << 1 /* "modify" */,
        Observe = 1 << 2 /* "observe" */,
        Remove = 1 << 3 /* "remove" */,
        HistoryObserve = 1 << 4 /* "history_observe" */
    };
}


class TAdministrativePermissions: public TAdministrativePermissionsImpl<NCommonAdministrative::EObjectAction, ui64> {
private:
    using TBase = TAdministrativePermissionsImpl<NCommonAdministrative::EObjectAction, ui64>;
public:
    using TBase::TBase;
};

class TBackgroundProcessesPermissions: public TAdministrativePermissions {
private:
    using TBase = TAdministrativePermissions;
    RTLINE_ACCEPTOR_DEF(TBackgroundProcessesPermissions, Types, TSet<TString>);
    RTLINE_ACCEPTOR_DEF(TBackgroundProcessesPermissions, Ids, TSet<TString>);
public:
    void FillPermissionsData(TMap<TString, TObjectActions>& actionsByType, TMap<TString, TObjectActions>& actionsById) const {
        for (auto&& i : Types) {
            actionsByType[i] |= GetActionsMask();
        }
        for (auto&& i : Ids) {
            actionsById[i] |= GetActionsMask();
        }
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        TJsonProcessor::WriteContainerArray(result, "types", Types);
        TJsonProcessor::WriteContainerArray(result, "ids", Ids);
        return result;
    }
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        if (!TBase::DeserializeFromJson(info)) {
            return false;
        }
        return TJsonProcessor::ReadContainer(info, "types", Types) &&
            TJsonProcessor::ReadContainer(info, "ids", Ids);
    }
};

class TSystemUserPermissions: public TDefaultUserPermissions {
private:
    using TBase = TDefaultUserPermissions;
    TMap<TString, TAdministrativePermissions::TObjectActions> BackgroundAbilitiesByType;
    TMap<TString, TAdministrativePermissions::TObjectActions> BackgroundAbilitiesById;
public:
    using TBase::TBase;
    using TPtr = TAtomicSharedPtr<TSystemUserPermissions>;
    virtual void AddAbilities(const TVector<TItemPermissionContainer>& abilities) override {
        TBase::AddAbilities(abilities);
        for (auto&& i : abilities) {
            if (auto item = i.GetPtrAs<TBackgroundProcessesPermissions>()) {
                item->FillPermissionsData(BackgroundAbilitiesByType, BackgroundAbilitiesById);
            }
        }
    }

    TAdministrativePermissions::TObjectActions GetBackgroundActionsByType(const TString& type) const {
        const TAdministrativePermissions::TObjectActions* p = TMapProcessor::GetValuePtr(BackgroundAbilitiesByType, type);
        if (p) {
            return *p;
        }
        return TMapProcessor::GetValueDef<TString, TAdministrativePermissions::TObjectActions>(BackgroundAbilitiesByType, "*", 0);
    }

    TAdministrativePermissions::TObjectActions GetBackgroundActionsById(const TString& id) const {
        const TAdministrativePermissions::TObjectActions* p = TMapProcessor::GetValuePtr(BackgroundAbilitiesById, id);
        if (p) {
            return *p;
        }
        return TMapProcessor::GetValueDef<TString, TAdministrativePermissions::TObjectActions>(BackgroundAbilitiesById, "*", 0);
    }
};

namespace NSearchAdministrative {
    enum class EObjectAction: ui64 {
        Add = 1 /* "add" */,
        Modify = 1 << 1 /* "modify" */,
        Observe = 1 << 2 /* "observe" */,
        Remove = 1 << 3 /* "remove" */,
        HistoryObserve = 1 << 4 /* "history_observe" */,
        ObserveAll = 1 << 5 /* "observe_all" */
    };
}

using TSearchAdministrativePermissions = TAdministrativePermissionsImpl<NSearchAdministrative::EObjectAction, ui64>;
