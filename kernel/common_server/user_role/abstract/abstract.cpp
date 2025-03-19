#include "abstract.h"

void IUserPermissions::AddAbilities(const TVector<TItemPermissionContainer>& abilities) {
    UserAbilities.insert(UserAbilities.end(), abilities.begin(), abilities.end());
    const auto pred = [](const TItemPermissionContainer& l, const TItemPermissionContainer& r) {
        return l->GetPriority() > r->GetPriority();
    };
    std::sort(UserAbilities.begin(), UserAbilities.end(), pred);
}

TItemPermissionContainer IUserPermissions::GetItemPermissions(const TString& item) const {
    for (auto&& i : UserAbilities) {
        if (item == i.GetItemId()) {
            return i;
        }
    }
    return TItemPermissionContainer();
}

NFrontend::TScheme TUserRolesCompiled::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme result;
    result.Add<TFSString>("system_user_id").SetNonEmpty(false);
    result.Add<TFSVariants>("role_names").SetVariants(server.GetRolesManager().GetRoleNames()).SetMultiSelect(true);
    return result;
}

NFrontend::TScheme TUserRolesCompiled::GetSearchScheme(const IBaseServer& /*server*/) {
    NFrontend::TScheme result;
    result.Add<TFSString>("system_user_id").SetNonEmpty(true);
    return result;
}

bool TUserRolesCompiled::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TJsonProcessor::Read(jsonInfo, "system_user_id", SystemUserId, true)) {
        return false;
    }
    if (!SystemUserId) {
        TFLEventLog::Log("incorrect system user id (empty)");
        return false;
    }
    if (!TJsonProcessor::ReadContainer(jsonInfo, "role_names", RoleNames)) {
        return false;
    }
    return true;
}

NJson::TJsonValue TUserRolesCompiled::SerializeToJson() const {
    NJson::TJsonValue result = NJson::JSON_MAP;
    TJsonProcessor::Write(result, "system_user_id", SystemUserId);
    TJsonProcessor::WriteContainerArray(result, "role_names", RoleNames);
    return result;
}

void TUserRolesCompiled::Merge(const TUserRolesCompiled& other) {
    RoleNames.insert(other.GetRoleNames().cbegin(), other.GetRoleNames().cend());
}

IPermissionsManager::IPermissionsManager(const IBaseServer& server, const IPermissionsManagerConfig& config)
        : IAutoActualization("PermissionsManager")
        , DefaultUser(config.GetDefaultUser())
        , Server(server)
        , UserInfoExtender(config.GetUserInfoExtender()->ConstructUserInfoExtender(Server))
{}

IPermissionsManager::TGetPermissionsContext::TGetPermissionsContext(const TString& userId, const TString& authModuleName /*= Default<TString>()*/)
    : UserId(userId)
    , AuthModuleName(authModuleName)
{}
