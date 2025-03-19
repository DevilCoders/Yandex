#include "item.h"

namespace NCS {

    TActionPermissions::TFactory::TRegistrator<TActionPermissions> TActionPermissions::Registrator(TActionPermissions::GetTypeName());

    void TActionsRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TActionPermissions>(permissions, TAdministrativePermissions::EObjectAction::Remove);
        TSet<TString> objectIds;
        ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "objects", objectIds), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_ids");
        ReqCheckCondition(GetServer().GetRolesManager().RemoveItems(objectIds, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot_remove_ids");
    }

    void TActionsUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TActionPermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);
        TVector<TItemPermissionContainer> objects;
        ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_objects");
        for (auto&& i : objects) {
            ReqCheckCondition(GetServer().GetRolesManager().UpsertItem(i, permissions->GetUserId(), nullptr), ConfigHttpStatus.UnknownErrorStatus, "cannot_upsert_object:" + i.GetItemId());
        }
    }

    void TActionsInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TActionPermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);
        TVector<TItemPermissionContainer> objects;
        ReqCheckCondition(GetServer().GetRolesManager().GetItems(objects), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_items");
        NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
        for (auto&& i : objects) {
            objectsJson.AppendValue(i.SerializeToJson());
        }
        g.MutableReport().AddReportElement("objects", std::move(objectsJson));
    }

}
