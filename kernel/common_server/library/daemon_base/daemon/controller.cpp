#include "controller.h"

#include <kernel/common_server/library/daemon_base/unistat_signals/signals.h>

#include <kernel/common_server/library/daemon_base/actions_engine/get_conf/action.h>
#include <kernel/common_server/library/daemon_base/metrics/metrics.h>
#include <kernel/common_server/library/daemon_base/metrics/persistent.h>
#include <kernel/daemon/protos/status.pb.h>
#include <kernel/common_server/util/system/dir_digest.h>

#include <kernel/daemon/common/time_guard.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <library/cpp/http/misc/httpcodes.h>

namespace {
    bool ReceiveJsonFields(const TCgiParameters& cgi, NJson::TJsonValue& fields, NJson::TJsonValue& absFields, TString& prefix) {
        prefix = cgi.Has("prefix") ? cgi.Get("prefix") : TString("server");
        prefix += ".";
        if (cgi.Has("fields")) {
            const TString fieldsStr = cgi.Get("fields");

            if (!NJson::ReadJsonTree(fieldsStr, &fields)) {
                ythrow yexception() << "incorrect fields parameter: " << fieldsStr;
            }
        } else {
            fields = NJson::JSON_UNDEFINED;
        }

        if (cgi.Has("abs_fields")) {
            const TString fieldsStr = cgi.Get("abs_fields");

            if (!NJson::ReadJsonTree(fieldsStr, &absFields)) {
                ythrow yexception() << "incorrect abs_fields parameter: " << fieldsStr;
            }
        } else {
            absFields = NJson::JSON_UNDEFINED;
        }
        return absFields.IsDefined() || fields.IsDefined();
    }

    bool ParseFieldParamGet(const TCgiParameters& cgi, NJson::TJsonValue& config, TString& prefix) {
        NJson::TJsonValue fields;
        NJson::TJsonValue absFields;
        if (!ReceiveJsonFields(cgi, fields, absFields, prefix)) {
            return false;
        }
        if (fields.IsDefined()) {
            NJson::TJsonValue::TArray arr;
            if (!fields.GetArray(&arr)) {
                ythrow yexception() << "incorrect fields parameter: " << fields;
            }
            for (auto&& i : arr) {
                config.AppendValue(prefix + i.GetStringRobust());
            }
        }

        if (absFields.IsDefined()) {
            NJson::TJsonValue::TArray arr;
            if (!absFields.GetArray(&arr)) {
                ythrow yexception() << "incorrect abs_fields parameter: " << absFields;
            }
            for (auto&& i : arr) {
                config.AppendValue(i.GetStringRobust());
            }
        }

        return true;
    }

    bool ParseFieldParamSet(const TCgiParameters& cgi, NJson::TJsonValue& config, TString& prefix) {
        NJson::TJsonValue fields;
        NJson::TJsonValue absFields;
        if (!ReceiveJsonFields(cgi, fields, absFields, prefix)) {
            return false;
        }

        if (fields.IsDefined()) {
            NJson::TJsonValue::TMapType map;
            if (!fields.GetMap(&map)) {
                ythrow yexception() << "incorrect fields parameter: " << fields;
            }
            for (auto&& i : map) {
                config[prefix + i.first] = i.second;
            }
        }

        if (absFields.IsDefined()) {
            NJson::TJsonValue::TMapType map;
            if (!absFields.GetMap(&map)) {
                ythrow yexception() << "incorrect fields parameter";
            }
            for (auto&& i : map) {
                config[i.first] = i.second;
            }
        }

        return true;
    }

    void PrepareConfig(TUnstrictConfig& result, const char* configStr) {
        if (!result.ParseMemory(configStr)) {
            TString errors;
            result.PrintErrors(errors);
            ythrow yexception() << "errors in server config: " << errors;
        }
    }
}

namespace NRTProc {
    bool TController::LoadConfigs(const bool force, const bool lastChance) {
        if (ConfigParams.GetDaemonConfig()->GetController().DMOptions.Enabled) {
            NController::TDownloadContext downloadContext(ConfigParams.GetDaemonConfig()->GetController().DMOptions);
            downloadContext.SetForceConfigsReading(force);
            const bool result = DownloadConfigsFromDM(downloadContext);
            CHECK_WITH_LOG(!lastChance || result || !StrictDMUsing()) << "Can't use DM for config restoring" << Endl;
            return result;
        } else {
            return NController::TController::LoadConfigs(force, lastChance);
        }
    }

    void TController::FillConfigsHashes(NJson::TJsonValue& result, TString configPath, const TRegExMatch* filter) const {
        if (!configPath)
            configPath = TFsPath(ConfigParams.GetPath()).Parent().GetPath();
        TDirHashInfo info = GetDirHashes(configPath, filter);
        for (const auto& i : info)
            result.InsertValue(i.first, i.second.Hash);
    }

    bool TController::CanSetConfig(bool& isExists) {
        TGuardServer gs = GetServer();
        isExists = !!gs;
        if (!isExists)
            return false;
        return Descriptor.CanSetConfig(gs);
    }

    bool TController::SetConfigFields(const TCgiParameters& cgiParams, NJson::TJsonValue& result) {
        NJson::TJsonValue config;
        TString prefix;
        if (!ParseFieldParamSet(cgiParams, config, prefix)) {
            return false;
        };
        const NJson::TJsonValue::TMapType* values;
        if (!config.GetMapPointer(&values))
            ythrow yexception() << "incorrect fields parameter";

        TUnstrictConfig serverConfig;
        TString confTextOld = Descriptor.GetConfigString(*this);
        PrepareConfig(serverConfig, confTextOld.data());
        bool configChanged = false;
        for (NJson::TJsonValue::TMapType::const_iterator i = values->begin(), e = values->end(); i != e; ++i) {
            const TString& path = i->first;
            const TString& value = i->second.GetString();
            bool fieldChanged = serverConfig.PatchEntry(path, value, "");
            configChanged |= fieldChanged;
            NJson::TJsonValue* fieldVal;
            if (path.StartsWith(prefix)) {
                fieldVal = &result.InsertValue(path.substr(prefix.size()), NJson::JSON_MAP);
            } else {
                fieldVal = &result.InsertValue(path, NJson::JSON_MAP);
            }
            fieldVal->InsertValue("value", value);
            fieldVal->InsertValue("result", fieldChanged);
        }
        if (!configChanged)
            return true;

        TStringStream configStrNew;
        serverConfig.PrintConfig(configStrNew);
        WriteFile(ConfigParams.GetPath(), configStrNew.Str());
        return true;
    }

    bool TController::GetConfigFields(const TCgiParameters& cgiParams, NJson::TJsonValue& result) {
        NJson::TJsonValue fields;
        TString prefix;
        if (!ParseFieldParamGet(cgiParams, fields, prefix)) {
            return false;
        };

        const NJson::TJsonValue::TArray* values;
        if (!fields.GetArrayPointer(&values))
            ythrow yexception() << "incorrect fields parameter";

        TUnstrictConfig serverConfig;
        TString confText = Descriptor.GetConfigString(*this);
        PrepareConfig(serverConfig, confText.data());

        for (NJson::TJsonValue::TArray::const_iterator i = values->begin(), e = values->end(); i != e; ++i) {
            const TString& path = i->GetString();
            const TYandexConfig::Section* section = serverConfig.GetSection(path);
            if (section)
                TUnstrictConfig::ToJsonPatch(*section, result, path);
            else {
                if (path.StartsWith(prefix)) {
                    result.InsertValue(path.substr(prefix.size()), serverConfig.GetValue(path));
                } else {
                    result.InsertValue(path, serverConfig.GetValue(path));
                }
            }
        }
        return true;
    }

    bool TController::DownloadConfigsFromDM(const NController::TDownloadContext& ctx) {
        TFsPath tempConfDir = ConfigParams.GetDaemonConfig()->GetController().StateRoot + "/configs_new";
        tempConfDir.ForceDelete();
        NDaemonController::TControllerAgent agent = ctx.CreateControllerAgent();
        NDaemonController::TListConfAction listConf = ctx.CreateListConfAction();

        if (!agent.ExecuteAction(listConf))
            return false;

        if (!ctx.CheckDownloadedService(listConf)) {
            MustBeAlive = false;
            return false;
        }

        for (auto& file : listConf.GetFileList()) {
            if (!ctx.CheckDeadline()) {
                WARNING_LOG << "Can't receive actual config files from DM: will try to start previous configuration" << Endl;
                return false;
            }
            {
                TTimeGuard timeGuard("Try to get file : url = " + file.Url + ", filename = " + file.Name);
                NDaemonController::TGetConfAction getConf(file, ctx.GetDeadline() - Now());
                if (!agent.ExecuteAction(getConf))
                    return false;
                file = getConf.GetData();
            }
        }
        for (auto& file : listConf.GetFileList())
            WriteFile(file.Name, file.Content, tempConfDir);
        TFsPath(ConfigParams.GetDaemonConfig()->GetController().ConfigsRoot).ForceDelete();
        tempConfDir.CopyTo(ConfigParams.GetDaemonConfig()->GetController().ConfigsRoot, true);
        tempConfDir.ForceDelete();
        return true;
    }

    TController::TController(TServerConfigConstructorParams& params, const IServerDescriptor& descriptor)
        : NController::TController(params, descriptor)
        , NoFileOperations(false)
        , MustBeAlive(true) {
        auto daemonConfig = params.GetDaemonConfig();
        if (daemonConfig->GetMetricsStorage()) {
            TFsPath(daemonConfig->GetMetricsStorage()).Parent().MkDirs();
        }

        SetMetricsPrefix(daemonConfig->GetMetricsPrefix());
        SetMetricsMaxAgeDays(daemonConfig->GetMetricsMaxAge());
        SetPersistentMetricsStorage(daemonConfig->GetMetricsStorage());
    }

    void TController::TExtendedClient::GetMetrics(IOutputStream& out) const {
        out << "HTTP/1.1 " << HttpCodeStrEx(HTTP_OK) << "\r\n\r\n";
        ::CollectMetrics(out);
    }

    void TController::TExtendedClient::ProcessServerStatus() {
        const TString origin = GetBaseRequestData().HeaderIn("Origin");
        const TString originHeader(!!origin ? ("Access-Control-Allow-Origin:" + origin + "\r\nAccess-Control-Allow-Credentials:true\r\n") : TString());
        Output()
            << "HTTP/1.1 200 Ok\r\n"sv
            << originHeader
            << "Content-Type: text/plain\r\n\r\n"sv;
        Output()
            << "Active                           : " << (Owner.GetStatus() == NController::ssbcActive) << Endl
            << "Must_be_alive                    : " << Owner.GetMustBeAlive() << Endl
            << "Non_Runnable                     : " << (Owner.GetStatus() == NController::ssbcNotRunnable) << Endl
            << "Search_Server_Running            : " << (Owner.GetStatus() == NController::ssbcActive) << Endl
            << "Status                           : " << Owner.GetStatus() << Endl
            ;
    }
}
