#include "handler.h"

IItemPermissions::TFactory::TRegistrator<TSettingsPermissions> TSettingsPermissions::Registrator(TSettingsPermissions::GetTypeName());

void TSettingsRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    ReqCheckPermissions<TSettingsPermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);
    auto& jsonElement = TJsonProcessor::GetAvailableElement(GetJsonData(), { "settings", "objects" });
    ReqCheckCondition(jsonElement.IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'settings' is not array");
    TVector<TString> settings;
    for (auto&& i : jsonElement.GetArraySafe()) {
        if (i.IsMap()) {
            ReqCheckCondition(i["setting_key"].IsString(), ConfigHttpStatus.SyntaxErrorStatus,
                              ELocalizationCodes::SyntaxUserError, "incorrect settings list");
            settings.emplace_back(i["setting_key"].GetString());
        } else {
            ReqCheckCondition(i.IsString(), ConfigHttpStatus.SyntaxErrorStatus,
                              ELocalizationCodes::SyntaxUserError, "incorrect settings list");
            settings.emplace_back(i.GetString());
        }
    }

    ReqCheckCondition(GetServer().GetSettings().RemoveKeys(settings, permissions->GetUserId()),ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot remove options");
    g.SetCode(HTTP_OK);
}


void TSettingsUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    ReqCheckPermissions<TSettingsPermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);

    auto& jsonElement = TJsonProcessor::GetAvailableElement(GetJsonData(), { "settings", "objects" });

    ReqCheckCondition(jsonElement.IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'settings' is not array");
    const TString& commonPrefix = GetHandlerSettingDef<TString>("common_prefix", "");
    TVector<NFrontend::TSetting> settings;
    TSet<TString> keys;
    for (auto&& i : jsonElement.GetArraySafe()) {
        NFrontend::TSetting setting;
        ReqCheckCondition(setting.DeserializeFromJson(i), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect settings record");
        setting.SetKey(commonPrefix + setting.GetKey());
        keys.emplace(commonPrefix + setting.GetKey());
        settings.emplace_back(setting);
    }

    TSet<TString> existKeys;
    ReqCheckCondition(GetServer().GetSettings().HasValues(keys, existKeys), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "");
    ReqCheckCondition(GetServer().GetSettings().SetValues(settings, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "incorrect settings processing");
    g.SetCode(HTTP_OK);
}


void TSettingsHistoryProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    ReqCheckPermissions<TSettingsPermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);

    TVector<TAtomicSharedPtr<TObjectEvent<NFrontend::TSetting>>> result;
    ReqCheckCondition(GetServer().GetSettings().GetHistory(TInstant::Zero(), result), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_history");
    NJson::TJsonValue settingsHistory(NJson::JSON_ARRAY);
    for (auto it = result.rbegin(); it != result.rend(); ++it) {
        settingsHistory.AppendValue((*it)->SerializeToJson());
    }
    g.MutableReport().AddReportElement("settings_history", std::move(settingsHistory));
    g.SetCode(HTTP_OK);
}


void TSettingsInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
    ReqCheckPermissions<TSettingsPermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);

    const TCgiParameters& cgi = Context->GetCgiParameters();
    const auto prefix = GetString(cgi, "prefix", false);
    TVector<NFrontend::TSetting> settings;

    ReqCheckCondition(GetServer().GetSettings().GetAllSettings(settings), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_settings");
    NJson::TJsonValue settingsJson(NJson::JSON_ARRAY);
    for (auto&& i : settings) {
        if (prefix && !i.GetKey().StartsWith(prefix)) {
            continue;
        }
        settingsJson.AppendValue(i.SerializeToJson());
    }
    auto settingsCopy = settingsJson;
    g.MutableReport().AddReportElement("settings", std::move(settingsJson));
    g.MutableReport().AddReportElement("objects", std::move(settingsCopy));
    g.SetCode(HTTP_OK);
}

