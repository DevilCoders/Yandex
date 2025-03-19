#include "role.h"
#include "manager.h"

NJson::TJsonValue TUserRoleInfo::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    TJsonProcessor::WriteContainerArray(result, "item_ids", ItemIds);
    TJsonProcessor::WriteContainerArray(result, "role_ids", RoleIds);
    return result;
}

bool TUserRoleInfo::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TBase::DeserializeFromJson(jsonInfo)) {
        return false;
    }
    if (!TJsonProcessor::ReadContainer(jsonInfo, "item_ids", ItemIds)) {
        return false;
    }
    if (!TJsonProcessor::ReadContainer(jsonInfo, "role_ids", RoleIds)) {
        return false;
    }
    return true;
}

NFrontend::TScheme TUserRoleInfo::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme result = TBase::GetScheme(server);
    result.Add<TFSVariants>("item_ids").SetVariants(server.GetRolesManager().GetItemIds()).SetMultiSelect(true);
    result.Add<TFSVariants>("role_ids").SetVariants(server.GetRolesManager().GetRoleNames()).SetMultiSelect(true).SetEditable(true);
    return result;
}

NJson::TJsonValue TUserRoleContainer::SerializeToJson() const {
    NJson::TJsonValue result = NJson::JSON_MAP;
    TJsonProcessor::Write(result, "role_name", RoleName);
    TJsonProcessor::Write(result, "revision", Revision);
    return result;
}

bool TUserRoleContainer::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TJsonProcessor::Read(jsonInfo, "role_name", RoleName, true)) {
        return false;
    }
    if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
        return false;
    }
    return true;
}

NFrontend::TScheme TUserRoleContainer::GetScheme(const IBaseServer& /*server*/) {
    NFrontend::TScheme result;
    result.Add<TFSString>("role_name").SetNonEmpty(true);
    result.Add<TFSNumeric>("revision").SetReadOnly(true);
    return result;
}
