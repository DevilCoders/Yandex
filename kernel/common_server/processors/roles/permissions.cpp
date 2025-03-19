#include "permissions.h"

namespace NCS {

    TRolePermissions::TFactory::TRegistrator<TRolePermissions> TRolePermissions::Registrator(TRolePermissions::GetTypeName());

    TString TRolePermissions::GetTypeName() {
        return "role";
    }
    TString TRolePermissions::GetClassName() const {
        return GetTypeName();
    }
    bool TRolePermissions::Check(const TString& roleName) const {
        return ManagedRoles.contains(roleName);
    }
    bool TRolePermissions::Check(const EObjectAction& action) const {
        return GetActions().contains(action);
    }
    NFrontend::TScheme TRolePermissions::DoGetScheme(const IBaseServer& server) const {
        NFrontend::TScheme result = TBase::DoGetScheme(server);
        result.Add<TFSVariants>("managed_roles").SetVariants(server.GetRolesManager().GetRoleNames()).SetMultiSelect(true);
        return result;
    }
    NJson::TJsonValue TRolePermissions::SerializeToJson() const {
        NJson::TJsonValue result = TBase::SerializeToJson();
        TJsonProcessor::WriteContainerArrayStrings(result, "managed_roles", ManagedRoles);
        return result;
    }
    bool TRolePermissions::DeserializeFromJson(const NJson::TJsonValue& info) {
        if (!TBase::DeserializeFromJson(info)) {
            return false;
        }
        return TJsonProcessor::ReadContainer(info, "managed_roles", ManagedRoles);
    }
}
