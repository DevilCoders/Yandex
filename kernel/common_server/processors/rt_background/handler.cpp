#include "handler.h"

#include <kernel/common_server/rt_background/manager.h>
#include <kernel/common_server/util/algorithm/container.h>

IItemPermissions::TFactory::TRegistrator<TRTBackgroundPermissions> TRTBackgroundPermissions::Registrator(TRTBackgroundPermissions::GetTypeName());

NFrontend::TScheme TRTBackgroundPermissions::DoGetScheme(const IBaseServer& server) const {
    NFrontend::TScheme result = TBase::DoGetScheme(server);
    result.Add<TFSVariants>("available_classes").InitVariantsClass<IRTBackgroundProcess>().AddVariant("*").SetMultiSelect(true);
    TMap<TString, TRTBackgroundProcessContainer> objects;
    server.GetRTBackgroundManager()->GetStorage().GetObjects(objects);
    result.Add<TFSVariants>("available_processes").SetVariants(MakeSet(NContainer::Keys(objects))).AddVariant("*").SetMultiSelect(true);
    result.Add<TFSArray>("available_owners").SetElement<TFSString>();
    return result;
}

void TRTBackgroundListProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    const TRTBackgroundManager* manager = GetServer().GetRTBackgroundManager();
    ReqCheckCondition(manager, ConfigHttpStatus.NotImplementedState, ELocalizationCodes::InternalServerError, "not configured");
    const TSet<TString> ids = MakeSet(GetStrings(Context->GetCgiParameters(), "ids", false));
    TMap<TString, TRTBackgroundProcessContainer> settings;
    TMap<TString, TRTBackgroundProcessStateContainer> states;
    ReqCheckCondition(manager->GetStorage().GetObjects(settings), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_infos");
    ReqCheckCondition(manager->GetStatesInfo(states), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_states");
    NJson::TJsonValue bgJson(NJson::JSON_ARRAY);
    TVector<const TRTBackgroundProcessContainer*> settingsVector;
    for (auto&& i : settings) {
        settingsVector.emplace_back(&i.second);
    }
    const auto pred = [](const TRTBackgroundProcessContainer* l, const TRTBackgroundProcessContainer* r) {
        return l->GetTitle() < r->GetTitle();
    };
    std::sort(settingsVector.begin(), settingsVector.end(), pred);

    for (auto&& i : settingsVector) {
        if (!ids.empty() && !ids.contains(i->GetName())) {
            continue;
        }
        if (!permissions->Check<TRTBackgroundPermissions>(TAdministrativePermissions::EObjectAction::Observe, *i)) {
            continue;
        }
        NJson::TJsonValue reportItem = i->GetReport();
        auto it = states.find(i->GetName());
        if (it != states.end()) {
            reportItem.InsertValue("background_process_state", it->second.GetReport());
        }
        bgJson.AppendValue(std::move(reportItem));
    }
    auto copy = bgJson;
    g.MutableReport().AddReportElement("rt_backgrounds", std::move(bgJson));
    g.MutableReport().AddReportElement("objects", std::move(copy));
    g.SetCode(HTTP_OK);
}


void TRTBackgroundSwitchingProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    const TRTBackgroundManager* manager = GetServer().GetRTBackgroundManager();
    ReqCheckCondition(manager, ConfigHttpStatus.NotImplementedState, ELocalizationCodes::InternalServerError, "not configured");
    auto& jsonElement = TJsonProcessor::GetAvailableElement(GetJsonData(), { "background_ids", "object_ids", "objects" });
    ReqCheckCondition(jsonElement.IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'backgrounds' is not array");
    bool enabled;
    ReqCheckCondition(GetJsonData()["enabled"].GetBoolean(&enabled), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'enabled' is not boolean");
    TVector<NFrontend::TSetting> backgrounds;
    TSet<TString> bgIds;

    TMap<TString, TRTBackgroundProcessContainer> settings;
    ReqCheckCondition(manager->GetStorage().GetObjects(settings), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_infos");

    for (auto&& i : jsonElement.GetArraySafe()) {
        TString processId;
        ReqCheckCondition(i.GetString(&processId), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect type in background_ids (must be string)");

        auto it = settings.find(processId);
        if (it != settings.end()) {
            ReqCheckCondition(permissions->Check<TRTBackgroundPermissions>(TAdministrativePermissions::EObjectAction::Modify, it->second), ConfigHttpStatus.PermissionDeniedStatus, ELocalizationCodes::NoPermissions);
        } else {
            continue;
        }

        bgIds.emplace(processId);
    }
    ReqCheckCondition(manager->GetStorage().SetBackgroundEnabled(bgIds, enabled, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot switch enabled");
    g.SetCode(HTTP_OK);
}

void TRTBackgroundUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    const TRTBackgroundManager* manager = GetServer().GetRTBackgroundManager();
    ReqCheckCondition(manager, ConfigHttpStatus.NotImplementedState, ELocalizationCodes::InternalServerError, "not configured");
    auto& jsonElement = TJsonProcessor::GetAvailableElement(GetJsonData(), { "backgrounds", "objects" });
    ReqCheckCondition(jsonElement.IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'backgrounds' is not array");
    const bool force = IsTrue(Context->GetCgiParameters().Get("force"));
    TVector<NFrontend::TSetting> backgrounds;
    for (auto&& i : jsonElement.GetArraySafe()) {
        TRTBackgroundProcessContainer container;
        ReqCheckCondition(container.DeserializeFromJson(i), ConfigHttpStatus.SyntaxErrorStatus, "incorrect background json");
        ReqCheckCondition(permissions->Check<TRTBackgroundPermissions>(TAdministrativePermissions::EObjectAction::Add, container), ConfigHttpStatus.PermissionDeniedStatus, ELocalizationCodes::NoPermissions);
        ReqCheckCondition(permissions->Check<TRTBackgroundPermissions>(TAdministrativePermissions::EObjectAction::Modify, container), ConfigHttpStatus.PermissionDeniedStatus, ELocalizationCodes::NoPermissions);
        if (force) {
            ReqCheckCondition(manager->GetStorage().ForceUpsertBackgroundSettings(container, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot force upsert bg");
        } else {
            ReqCheckCondition(manager->GetStorage().UpsertBackgroundSettings(container, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot upsert bg");
        }
    }
    g.SetCode(HTTP_OK);
}

void TRTBackgroundRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    const TRTBackgroundManager* manager = GetServer().GetRTBackgroundManager();
    ReqCheckCondition(manager, ConfigHttpStatus.NotImplementedState, ELocalizationCodes::InternalServerError, "not configured");
    auto& jsonElement = TJsonProcessor::GetAvailableElement(GetJsonData(), { "background_ids", "object_ids", "ids", "objects" });
    ReqCheckCondition(jsonElement.IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'ids' is not array");
    TMap<TString, TRTBackgroundProcessContainer> settings;
    ReqCheckCondition(manager->GetStorage().GetObjects(settings), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_infos");
    TVector<TString> ids;
    for (auto&& i : jsonElement.GetArraySafe()) {
        auto it = settings.find(i.GetString());
        ReqCheckCondition(it != settings.end(), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, ToString("unknown id ") + i.GetString());
        ReqCheckCondition(permissions->Check<TRTBackgroundPermissions>(TAdministrativePermissions::EObjectAction::Remove, it->second), ConfigHttpStatus.PermissionDeniedStatus, ELocalizationCodes::NoPermissions);
        ids.emplace_back(i.GetString());
    }
    ReqCheckCondition(manager->GetStorage().RemoveBackground(ids, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot remove options");
    g.SetCode(HTTP_OK);
}

