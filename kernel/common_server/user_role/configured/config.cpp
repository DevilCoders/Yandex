#include "config.h"

TConfiguredPermissionsManagerConfig::TFactory::TRegistrator<TConfiguredPermissionsManagerConfig> TConfiguredPermissionsManagerConfig::Registrator(TConfiguredPermissionsManagerConfig::GetTypeName());

void TConfiguredPermissionsManagerConfig::DoInit(const TYandexConfig::Section* section) {
    auto children = section->GetAllChildren();
    auto it = children.find("Users");
    AssertCorrectConfig(it != children.end(), "No Users in configuration");
    for (auto&& i : it->second->GetAllChildren()) {
        StringSplitter(i.second->GetDirectives().Value<TString>("Roles", "")).SplitBySet(", ").SkipEmpty().Collect(&UserRoles[i.first]);
    }
    AssertCorrectConfig(UserRoles.contains(GetDefaultUser()), "No roles for DefaultUser");
}

void TConfiguredPermissionsManagerConfig::DoToString(IOutputStream& os) const {
    os << "<Users>" << Endl;
    for (auto&& i : UserRoles) {
        os << "<" << i.first << ">" << Endl;
        os << "Roles: " << JoinSeq(", ", i.second) << Endl;
        os << "</" << i.first << ">" << Endl;
    }
    os << "</Users>" << Endl;
}

bool TConfiguredPermissionsManager::Restore(const TString& systemUserId, TUserRolesCompiled& result) const {
    auto it = UserRoles.find(systemUserId);
    result.SetSystemUserId(systemUserId);
    if (it != UserRoles.end()) {
        result.SetRoleNames(it->second);
    }
    return true;
}

bool TConfiguredPermissionsManager::Upsert(const TUserRolesCompiled& /*result*/, const TString& /*userId*/, bool* /*isUpdate*/) const {
    return false;
}
