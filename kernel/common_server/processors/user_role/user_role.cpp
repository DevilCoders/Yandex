#include "user_role.h"
#include <kernel/common_server/user_auth/abstract/abstract.h>

namespace NCS {

    TUserRolePermissions::TFactory::TRegistrator<TUserRolePermissions> TUserRolePermissions::Registrator(TUserRolePermissions::GetTypeName());

    void TUserRolesUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TUserRolePermissions>(permissions, TSearchAdministrativePermissions::EObjectAction::Add);
        TVector<TUserRolesCompiled> objects;
        ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_objects");
        for (auto&& i : objects) {
            ReqCheckCondition(GetServer().GetPermissionsManager().Upsert(i, permissions->GetUserId(), nullptr), ConfigHttpStatus.UnknownErrorStatus, "cannot_upsert_object:" + i.GetSystemUserId());
        }
    }

    void TUserRolesInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TUserRolePermissions>(permissions, TSearchAdministrativePermissions::EObjectAction::Observe);
        ReqCheckCondition(GetJsonData().Has("system_user_id"), ConfigHttpStatus.SyntaxErrorStatus, "no system_user_id");
        const TString& systemUserId = GetJsonData()["system_user_id"].GetStringRobust();
        NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
        if (systemUserId == "*") {
            ReqCheckPermissions<TUserRolePermissions>(permissions, TSearchAdministrativePermissions::EObjectAction::ObserveAll);
            TFLEventLog::Info("user_role handler request without system_user_id parameter");
            TVector<TUserRolesCompiled> objectLinks;
            ReqCheckCondition(GetServer().GetPermissionsManager().RestoreAll(objectLinks), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_items");
            for (auto&& i : objectLinks) {
                objectsJson.AppendValue(i.SerializeToJson());
            }
        } else if (!systemUserId.empty()) {
            TUserRolesCompiled objectLink;
            ReqCheckCondition(GetServer().GetPermissionsManager().Restore(systemUserId, objectLink), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_items");
            objectsJson.AppendValue(objectLink.SerializeToJson());
        }
        g.MutableReport().AddReportElement("objects", std::move(objectsJson));
    }

}
