#include "user_auth.h"
#include <kernel/common_server/user_auth/abstract/abstract.h>

namespace NCS {

    TUserAuthPermissions::TFactory::TRegistrator<TUserAuthPermissions> TUserAuthPermissions::Registrator(TUserAuthPermissions::GetTypeName());

    void TAuthUsersRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TUserAuthPermissions>(permissions, TSearchAdministrativePermissions::EObjectAction::Remove);
        TVector<ui32> objectIds;
        ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "objects", objectIds), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_ids");
        ReqCheckCondition(GetServer().GetAuthUsersManager().Remove(objectIds, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot_remove_ids");
    }

    void TAuthUsersUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TUserAuthPermissions>(permissions, TSearchAdministrativePermissions::EObjectAction::Add);
        TVector<TAuthUserLink> objects;
        ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_objects");
        for (auto&& i : objects) {
            ReqCheckCondition(GetServer().GetAuthUsersManager().Upsert(i, permissions->GetUserId(), nullptr), ConfigHttpStatus.UnknownErrorStatus, "cannot_upsert_object:" + i.GetAuthUserId());
        }
    }

    void TAuthUsersInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TUserAuthPermissions>(permissions, TSearchAdministrativePermissions::EObjectAction::Observe);
        TVector<TAuthUserLink> objectLinks;
        if (GetJsonData().Has("auth_id")) {
            TAuthUserLinkId authId;
            ReqCheckCondition(authId.DeserializeFromJson(GetJsonData()["auth_id"]), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_auth_id");
            TMaybe<TAuthUserLink> objectLink;
            ReqCheckCondition(GetServer().GetAuthUsersManager().Restore(authId, objectLink), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_items");
            if (!!objectLink) {
                objectLinks.emplace_back(*objectLink);
            }
        } else if (GetJsonData().Has("system_user_id")) {
            ReqCheckCondition(GetServer().GetAuthUsersManager().Restore(GetJsonData()["system_user_id"].GetStringRobust(), objectLinks), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_items");
        } else {
            ReqCheckPermissions<TUserAuthPermissions>(permissions, TSearchAdministrativePermissions::EObjectAction::ObserveAll);
            ReqCheckCondition(GetServer().GetAuthUsersManager().RestoreAll(objectLinks), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_items");
        }
        NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
        TJsonProcessor::WriteObjectsArray(objectsJson, objectLinks);
        g.MutableReport().AddReportElement("objects", std::move(objectsJson));
    }

}
