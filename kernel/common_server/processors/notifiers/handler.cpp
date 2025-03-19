#include "handler.h"
#include <kernel/common_server/notifications/manager.h>

IItemPermissions::TFactory::TRegistrator<TNotifierPermissions> TNotifierPermissions::Registrator(TNotifierPermissions::GetTypeName());

void TNotifiersRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    ReqCheckPermissions<TNotifierPermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);
    ReqCheckCondition(!!GetServer().GetNotifiersManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "notifications not configured");

    ReqCheckCondition(GetJsonData()["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'objects' is not array");
    TSet<TString> objectIds;
    for (auto&& i : GetJsonData()["objects"].GetArraySafe()) {
        ReqCheckCondition(i.IsString(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects list");
        objectIds.emplace(i.GetString());
    }

    auto session = GetServer().GetNotifiersManager()->BuildNativeSession(false);
    if (!GetServer().GetNotifiersManager()->RemoveObject(objectIds, permissions->GetUserId(), session) || !session.Commit()) {
        session.DoExceptionOnFail(ConfigHttpStatus);
    }

    g.SetCode(HTTP_OK);
}

void TNotifiersUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    ReqCheckPermissions<TNotifierPermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);
    ReqCheckCondition(!!GetServer().GetNotifiersManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "notifications not configured");


    ReqCheckCondition(GetJsonData()["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'objects' is not array");
    TVector<TNotifierContainer> objects;
    TSet<TString> keys;
    for (auto&& i : GetJsonData()["objects"].GetArraySafe()) {
        TNotifierContainer object;
        ReqCheckCondition(TBaseDecoder::DeserializeFromJson(object, i), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect object record");
        keys.emplace(object.GetInternalId());
        objects.emplace_back(std::move(object));
    }

    auto session = GetServer().GetNotifiersManager()->BuildNativeSession(false);
    for (auto&& i : objects) {
        if (!GetServer().GetNotifiersManager()->ForceUpsertObject(i, permissions->GetUserId(), session)) {
            session.DoExceptionOnFail(ConfigHttpStatus);
        }
    }
    if (!session.Commit()) {
        session.DoExceptionOnFail(ConfigHttpStatus);
    }
    g.SetCode(HTTP_OK);
}

void TNotifiersInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    ReqCheckPermissions<TNotifierPermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);

    const TCgiParameters& cgi = Context->GetCgiParameters();
    const auto actuality = GetTimestamp(cgi, "actuality", Context->GetRequestStartTime());
    TMap<TString, TNotifierContainer> objects;

    ReqCheckCondition(!!GetServer().GetNotifiersManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "notifications not configured");
    ReqCheckCondition(GetServer().GetNotifiersManager()->GetObjects(objects, actuality), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_settings");
    NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
    for (auto&& i : objects) {
        objectsJson.AppendValue(i.second.SerializeToJson());
    }
    g.MutableReport().AddReportElement("objects", std::move(objectsJson));
    g.SetCode(HTTP_OK);
}

