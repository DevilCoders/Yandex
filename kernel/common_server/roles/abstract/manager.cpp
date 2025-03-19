#include "manager.h"

bool IRolesManager::UpsertRole(const TUserRoleInfo& /*role*/, const TString& /*userId*/, bool* /*isUpdate*/ /*= nullptr*/) const {
    return false;
}

bool IRolesManager::RemoveRoles(const TSet<TString>& /*roleNames*/, const TString& /*userId*/) const {
    return false;
}

bool IRolesManager::GetRoles(TVector<TUserRoleInfo>& /*roles*/) const {
    return false;
}

bool IRolesManager::UpsertItem(const TItemPermissionContainer& /*item*/, const TString& /*userId*/, bool* /*isUpdate*/ /*= nullptr*/) const {
    return false;
}

bool IRolesManager::RemoveItems(const TSet<TString>& /*itemNames*/, const TString& /*userId*/) const {
    return false;
}

bool IRolesManager::GetItems(TVector<TItemPermissionContainer>& /*items*/) const {
    return false;
}

TSet<TString> IRolesManager::GetRoleNames() const {
    TVector<TUserRoleInfo> roles;
    GetRoles(roles);
    TSet<TString> result;
    for (auto&& i : roles) {
        result.emplace(i.GetRoleName());
    }
    return result;
}

TSet<TString> IRolesManager::GetItemIds() const {
    TVector<TItemPermissionContainer> items;
    GetItems(items);
    TSet<TString> result;
    for (auto&& i : items) {
        result.emplace(i.GetItemId());
    }
    return result;
}
