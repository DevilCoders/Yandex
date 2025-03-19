#include "controller.h"

#include <kernel/common_server/library/daemon_base/actions_engine/controller_script.h>
#include <kernel/common_server/library/daemon_base/metrics/servicemetrics.h>
#include <kernel/common_server/util/network/http_request.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/stream/zlib.h>
#include <util/string/type.h>

#define DEFINE_COMMON_CONTROLLER_COMMAND(name) \
class T ## name ## Command : public NRTProc::TController::TExtendedCommandProcessor {\
protected:\
    virtual TString GetName() const { return #name ; }\
    virtual void DoProcess();\
};\
    NRTProc::TController::TCommandProcessor::TFactory::TRegistrator<T ## name ## Command> Registrator ## name(#name);\
    void T ## name ## Command::DoProcess() {

DEFINE_COMMON_CONTROLLER_COMMAND(no_file_operations)
    TString value = GetCgi().Get("set");
    if (!!value)
        Owner->SetNoFileOperations(FromString<bool>(value));
    Write("result", Owner->GetNoFileOperations());
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(get_async_command_info)
    TString taskId = GetCgi().Get("id");
    Owner->GetAsyncCommandInfo(taskId, *this);
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(put_file)
    const TString filename = GetCgi().Get("filename");
    if (!filename)
        throw yexception() << "there is no filename";
    TStringBuf data = GetPostBuffer();
    Owner->WriteFile(filename, data);
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(take_file)
    const TString& filename = GetCgi().Get("filename");
    if (!filename)
        throw yexception() << "there is no filename";
    const TString& fromHost = GetCgi().Get("from_host");
    if (!fromHost)
        throw yexception() << "there is no from_host";
    const TString& fromPortStr = GetCgi().Get("from_port");
    if (!fromPortStr)
        throw yexception() << "there is no from_port";
    ui16 fromPort = FromString<ui16>(fromPortStr);
    ui64 sleepDurationMs = 100;
    if (GetCgi().Has("sleep_duration_ms"))
        sleepDurationMs = FromString<ui64>(GetCgi().Get("sleep_duration_ms"));
    const TString& url = Base64Decode(GetCgi().Get("url"));
    NUtil::THttpRequest request(url, TString(GetPostBuffer()));
    request.SetSleepingPause(sleepDurationMs);
    request.SetAttemptionsMax(5);
    NUtil::THttpReply reply;
    reply = request.Execute(fromHost, fromPort);
    if (reply.IsSuccessReply()) {
        Owner->WriteFile(filename, reply.Content());
        return;
    }
    ythrow yexception() << "error while take file: " << reply.ErrorMessage() << "/" << reply.Content();
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(delete_file)
    const TString filename = GetCgi().Get("filename");
    if (!filename)
        throw yexception() << "there is no filename";
    Owner->GetFullPath(filename).ForceDelete();
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(get_configs_hashes)
    NJson::TJsonValue result(NJson::JSON_MAP);
    TString path(GetCgi().Get("path"));
    if (GetCgi().Has("filter")) {
        const TString& filterStr = GetCgi().Get("filter");
        if (filterStr) {
            TRegExMatch filter(filterStr.data());
            Owner->FillConfigsHashes(result, path, &filter);
        }
    } else
        Owner->FillConfigsHashes(result, path, nullptr);
    Write("result", result);
END_CONTROLLER_COMMAND

namespace {
    class TScriptLogger : public NDaemonController::IControllerAgentCallback {
    public:
        TScriptLogger(NRTProc::TController::TCommandProcessor& owner)
            : Result(NJson::JSON_ARRAY)
            , Owner(owner)
            , CurrentAction(nullptr)
        {}
        virtual void OnAfterActionStep(const TActionContext& /*context*/, NDaemonController::TAction& action) override {
            CHECK_WITH_LOG(CurrentAction);
            *CurrentAction = action.SerializeExecutionInfo();
            Owner.Write("script", Result);
        }
        virtual void OnBeforeActionStep(const TActionContext& /*context*/, NDaemonController::TAction& action) override {
            CurrentAction = &Result.AppendValue(NJson::JSON_MAP);
            *CurrentAction = action.SerializeExecutionInfo();
            Owner.Write("script", Result);
        }
    private:
        NJson::TJsonValue Result;
        NRTProc::TController::TCommandProcessor& Owner;
        NJson::TJsonValue* CurrentAction;
    };
}

DEFINE_COMMON_CONTROLLER_COMMAND(execute_script)
    TString scriptName = GetCgi().Get("script_name");
    TString decoded = Base64Decode(GetPostBuffer());
    TStringInput si(decoded);
    TZLibDecompress decompressed(&si);
    NJson::TJsonValue scriptJson;
    if (!NJson::ReadJsonTree(&decompressed, &scriptJson))
        ythrow yexception() << "Errors in script " << scriptName << " json: " << decompressed.ReadAll();
    NJson::TJsonValue result;
    TScriptLogger logger(*this);
    NRTYScript::TScript script;
    if (!script.Deserialize(scriptJson))
        ythrow yexception() << "cannot deserialize script " << scriptName;
    result.InsertValue("name", scriptName);
    DEBUG_LOG << "Execute script " << NJson::WriteJson(&scriptJson, true, true) << Endl;
    Owner->ClearServerDataStatus();
    script.Execute(16);
    scriptJson = script.Serialize();
    if (script.TasksCount(NRTYScript::TTaskContainer::StatusFailed)) {
        ERROR_LOG << "Execute script result: " << NJson::WriteJson(&scriptJson, true, true) << Endl;
        result.InsertValue("info", scriptJson);
        Write("result", result);
        ythrow yexception() << "script failed";
    } else {
        DEBUG_LOG << "Execute script result: " << NJson::WriteJson(&scriptJson, true, true) << Endl;
        result.InsertValue("info", script.GetStatusInfo());
        Write("result", result);
    }
    auto daemonConfig = Owner->GetDaemonConfig();
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(set_config)
    bool isExists;
    bool canSetConfig = Owner->CanSetConfig(isExists);
    if(isExists && !canSetConfig)
        ythrow yexception() << "Incorrect set_config situation";
    NJson::TJsonValue result(NJson::JSON_MAP);
    if (!Owner->SetConfigFields(GetCgi(), result)) {
        Write("result", "");
        return;
    };
    Write("result", result);

    Owner->RestartServer(NController::TRestartContext(0));

    if (Owner->GetStatus() == NController::ssbcNotRunnable || Owner->IsRestored()) {
        ythrow yexception() << Owner->GetStatus() << "/" << Owner->IsRestored();
    }
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(get_config)
    NJson::TJsonValue result(NJson::JSON_MAP);
    if (!Owner->GetConfigFields(GetCgi(), result)) {
        Write("result", "");
        return;
            }
    Write("result", result);
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(get_metric)
    if (IsTrue(GetCgi().Get("update"))) {
        GetGlobalMetrics().UpdateValues(true);
    }

    const TString& name = GetCgi().Get("name");
    auto result = GetMetricResult(name);
    if (!result.Defined()) {
        result = GetMetricResult(GetMetricsPrefix() + name);
    }
    if (!result.Defined()) {
        Write("error", "undefined metric");
        return;
    }

    NJson::TJsonValue rates;
    rates.InsertValue("average", result->AvgRate);
    rates.InsertValue("current", result->CurRate);
    rates.InsertValue("maximum", result->MaxRate);
    rates.InsertValue("minimum", result->MinRate);
    Write("value", result->Value);
    Write("rate", rates);
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(get_must_be_alive)
    Write("must_be_alive", Owner->GetMustBeAlive());
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(download_configs_from_dm)
    if (!GetCgi().Has("dm_options"))
        ythrow yexception() << "there is no dm_options";
    TDaemonConfig::TControllerConfig::TDMOptions dm;
    dm.Deserialize(NUtil::JsonFromString(GetCgi().Get("dm_options")));
    if (!dm.Slot && Owner->GetDaemonConfig()) {
        dm.Slot = Owner->GetDaemonConfig()->GetController().DMOptions.Slot;
    }
    const TString& version = GetCgi().Get("config_version");
    TString serviceType;
    if (IsTrue(GetCgi().Get("force"))) {
        Y_ENSURE(GetCgi().Has("service_type"));
        serviceType = GetCgi().Get("service_type");
    } else {
        Y_ENSURE(!GetCgi().Has("service_type"));
    }
    NController::TDownloadContext downloadContext(dm);
    downloadContext.SetForceConfigsReading(IsTrue(GetCgi().Get("force"))).SetVersion(version).SetServiceType(serviceType);
    Write("downloaded", Owner->DownloadConfigsFromDM(downloadContext));
END_CONTROLLER_COMMAND

#undef DEFINE_COMMON_CONTROLLER_COMMAND
