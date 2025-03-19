#include "download_configs_from_dm.h"

namespace NDaemonController {

    #define DOWNLOAD_CONFIGS_FROM_DM_ACTION_NAME "DOWNLOAD_CONFIGS_FROM_DM"

    void TDownloadConfigsFromDmAction::AddPrevActionsResult(const NRTYScript::ITasksInfo& info) {
        if (ConfigVersion.StartsWith('$')) {
            TString path = ConfigVersion.substr(1);
            NJson::TJsonValue result;
            if (info.GetValueByPath(path, result) && !!result.GetStringRobust()) {
                ConfigVersion = result.GetStringRobust();
            } else {
                DEBUG_LOG << "Incorrect extract data path " << path << Endl;
            }
        }
    }

    TDownloadConfigsFromDmAction::TDownloadConfigsFromDmAction(const TString& waitActionName)
        : TControllerAsyncAction(waitActionName)
    {}

    TDownloadConfigsFromDmAction::TDownloadConfigsFromDmAction(TAsyncPolicy policy, const TString& configVersion)
        : TControllerAsyncAction(policy)
        , ConfigVersion(configVersion)
    {
        DMOptions.Enabled = true;
    }

    TDownloadConfigsFromDmAction::TDownloadConfigsFromDmAction()
    {}

    NDaemonController::TAction::TLockType TDownloadConfigsFromDmAction::GetLockType() const {
        return ltWriteLock;
    }

    bool TDownloadConfigsFromDmAction::GetNotContinuableTaskOnStarting() const {
        return false;
    }

    TString TDownloadConfigsFromDmAction::ActionName() const {
        return DOWNLOAD_CONFIGS_FROM_DM_ACTION_NAME;
    }

    TDaemonConfig::TControllerConfig::TDMOptions& TDownloadConfigsFromDmAction::MutableDMOptions() {
        return DMOptions;
    }

    TDownloadConfigsFromDmAction& TDownloadConfigsFromDmAction::SetForce(const bool force) {
        Force = force;
        return *this;
    }

    TDownloadConfigsFromDmAction& TDownloadConfigsFromDmAction::SetServiceType(const TString& serviceType) {
        ServiceType = serviceType;
        return *this;
    }

    NJson::TJsonValue TDownloadConfigsFromDmAction::DoSerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("dm_options", DMOptions.Serialize());
        result.InsertValue("config_version", ConfigVersion);
        result.InsertValue("force", Force);
        if (Force) {
            result.InsertValue("service_type", ServiceType);
        }
        return result;
    }

    void TDownloadConfigsFromDmAction::DoDeserializeFromJson(const NJson::TJsonValue& json) {
        DMOptions.Deserialize(json["dm_options"]);
        ConfigVersion = json["config_version"].GetStringRobust();
        Force = json["force"].GetBooleanRobust();
        if (Force) {
            ServiceType = json["service_type"].GetStringRobust();
        }
    }

    TString TDownloadConfigsFromDmAction::DoBuildCommandStart() const {
        CHECK_WITH_LOG(!ConfigVersion.StartsWith('$') && !!ConfigVersion);
        return "command=download_configs_from_dm"
               "&async=yes&dm_options=" + DMOptions.Serialize().GetStringRobust() +
               "&config_version=" + ConfigVersion +
               "&force=" + ToString(Force) +
               (Force ? "&service_type=" + ServiceType : "");
    }

    void TDownloadConfigsFromDmAction::InterpretTaskReply(TAsyncTaskExecutor::TTask::TStatus taskStatus, const NJson::TJsonValue& result) {
        CHECK_WITH_LOG(!ConfigVersion.StartsWith('$') && !!ConfigVersion);
        const NJson::TJsonValue::TMapType& map = result.GetMap();
        NJson::TJsonValue::TMapType::const_iterator i = map.find("downloaded");
        if (i != map.end() && !i->second.GetBooleanRobust())
            Fail("Hasn't downloaded: " + result.GetStringRobust());
        else
            TControllerAsyncAction::InterpretTaskReply(taskStatus, result);
    }

    TDownloadConfigsFromDmAction::TFactory::TRegistrator<TDownloadConfigsFromDmAction> TDownloadConfigsFromDmAction::Registrator(DOWNLOAD_CONFIGS_FROM_DM_ACTION_NAME);

}
