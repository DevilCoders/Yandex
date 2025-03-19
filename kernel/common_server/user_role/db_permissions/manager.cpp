#include "manager.h"
#include <kernel/common_server/roles/db_roles/manager.h>

bool TDBPermissionsManager::Restore(const TString& systemUserId, TUserRolesCompiled& result) const {
    const TSet<ui32> roleIds = Links->GetSlaves(systemUserId);
    const TSet<TString> roleNames = Server.GetRolesManager().RestoreRoleNames(roleIds);
    result.SetSystemUserId(systemUserId).SetRoleNames(roleNames);
    return true;
}

bool TDBPermissionsManager::RestoreAll(TVector<TUserRolesCompiled>& result) const {
    const auto indexByOwnerData = Links->DetachIndexByOwnerId().GetIndexData();
    for (const auto& [systemUserId, slaves] : indexByOwnerData) {
        TUserRolesCompiled userRole;
        TSet<ui32> roleIds;
        for (auto&& i : slaves) {
            roleIds.emplace(i.GetSlaveId());
        }
        const TSet<TString> roleNames = Server.GetRolesManager().RestoreRoleNames(roleIds);
        userRole.SetSystemUserId(systemUserId).SetRoleNames(roleNames);
        result.push_back(std::move(userRole));
    }
    return true;
}

bool TDBPermissionsManager::Upsert(const TUserRolesCompiled& result, const TString& userId, bool* isUpdate) const {
    if (isUpdate) {
        *isUpdate = false;
    }
    auto session = Links->BuildNativeSession(false);
    if (!Links->RemoveByOwnerId({ result.GetSystemUserId() }, userId, session)) {
        return false;
    }

    const TDBFullRolesManager* dbRolesManager = dynamic_cast<const TDBFullRolesManager*>(&Server.GetRolesManager());
    if (!dbRolesManager) {
        TFLEventLog::Log("roles manager is not compatible");
        return false;
    }
    TVector<TDBRole> roles;
    if (!dbRolesManager->GetRolesManager().RestoreByRoleName(result.GetRoleNames(), roles, session)) {
        return false;
    }
    TSet<ui32> roleIds;
    for (auto&& i : roles) {
        roleIds.emplace(i.GetRoleId());
    }

    if (!Links->AddLinks(result.GetSystemUserId(), roleIds, userId, session)) {
        return false;
    }
    return session.Commit();
}

TDBPermissionsManager::TDBPermissionsManager(const TString& dbName, const THistoryConfig& hConfig, const IPermissionsManagerConfig& config, const IBaseServer& server)
    : TBase(server, config)
    , Links(MakeHolder<TLinks>(server.GetDatabase(dbName), hConfig))
{

}
