#include "base_controller.h"
#include "messages.h"
#include "common/time_guard.h"
#include "bomb.h"

#include <library/cpp/json/json_writer.h>
#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <library/cpp/balloc/optional/operators.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/string_utils/tskv_format/builder.h>

namespace NController {

    //TController
    bool TController::Process(IMessage* message) {
        TCollectServerInfo* msg = dynamic_cast<TCollectServerInfo*>(message);
        if (msg) {
            TInstant now = Now();
            msg->Fields["controller_uptime"] = (now - ControllerStart).Seconds();
            msg->Fields["controller_status"] = ToString(GetStatus());
            msg->Fields["server_uptime"] = ServerStart != TInstant::Zero() ? ToString(now - ServerStart) : "0";
            try {
                TUnstrictConfig::ToJson(Descriptor.GetConfigString(*this), msg->Fields["config"]);
            } catch (...) {
                msg->Fields["config"] = NJson::JSON_UNDEFINED;
            }
            return true;
        }
        TSystemStatusMessage* msgStatusData = dynamic_cast<TSystemStatusMessage*>(message);
        if (msgStatusData) {
            if (!!EnvironmentManager) {
                EnvironmentManager->SetStatus(msgStatusData->GetMessage(), msgStatusData->GetStatus());
            }
            return true;
        }
        return false;
    }

    TServerInfo TController::GetServerInfo(bool controllerOnly) {
        TAutoPtr<TCollectServerInfo> msg(Descriptor.CreateInfoCollector());
        NJson::TJsonValue statusInfo;
        if (!!EnvironmentManager) {
            statusInfo = EnvironmentManager->BuildReport();
        }
        statusInfo["timestamp"] = ToString(Now().GetValue());
        DEBUG_LOG << "Status info: " << statusInfo.GetStringRobust() << Endl;
        if (!controllerOnly) {
            TServerInfo info = CollectServerInfo(msg);
            info.InsertValue("server_status_global", statusInfo);
            return info;
        } else {
            TServerInfo info;
            info.InsertValue("server_status_global", statusInfo);
            return info;
        }
    }

    bool NController::TController::LoadConfigs(bool /* force */, const bool /*lastChance*/) {
        return false;
    }

    bool TController::StartServerImpl(const TRestartContext& ctx) noexcept {
        CHECK_WITH_LOG(!Server);
        CHECK_WITH_LOG(!!Config);
        if (ConfigParams.GetDaemonConfig()->GetController().StartServer) {
            TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> timeGuard(ctx.MutableStartTimePtr(), "Create and start Server");
            SetStatus(NController::ssbcStarting);
            try {
                {
                    auto conf = GetConfig();
                    Server.Reset(new TExtendedServer(THolder<NController::IServer>(Descriptor.CreateServer(*conf)), *conf));
                }
                Server->Run();
            } catch (...) {
                ERROR_LOG << CurrentExceptionMessage() << Endl;
                SetStatus(NController::ssbcStopped);
                Server = nullptr;
                return false;
            }
            SetStatus(NController::ssbcActive);
            ServerStart = Now();
        } else {
            NOTICE_LOG << "Controller.StartServer is false" << Endl;
            SetStatus(NController::ssbcStopped);
            return false;
        }
        return true;
    }

    void TController::RereadBaseConfig() {
        ConfigParams.RefreshConfig();

        if (ConfigParams.GetDaemonConfig()->GetController().ConfigsControl) {
            if (!EnvironmentManager) {
                EnvironmentManager.Reset(new TEnvironmentManager(ConfigParams));
            }
        } else if (!!EnvironmentManager) {
            EnvironmentManager.Destroy();
        }

        if (ConfigParams.GetDaemonConfig()->GetController().ReinitLogsOnRereadConfigs) {
            TWriteGuard guard(LogMutex);
            Log = MakeAtomicShared<TLog>(ConfigParams.GetDaemonConfig()->GetController().Log);
        }
    }

    bool TController::InitConfiguration(const TRestartContext& ctx) noexcept {
        CHECK_WITH_LOG(!Server);

        {
            TTimeGuard timeGuard("Loading configs");
            bool successfullyLoad = false;
            const ui32 attemptionsCount = ConfigParams.GetDaemonConfig()->GetController().GetLoadConfigsAttemptionsCount();
            for (ui32 att = 0; att < ConfigParams.GetDaemonConfig()->GetController().GetLoadConfigsAttemptionsCount(); ++att) {
                try {
                    if (LoadConfigs(ctx.GetForceConfigsReading(), att + 1 == attemptionsCount)) {
                        INFO_LOG << "Configuration loaded on " << att + 1 << " attemption successfully" << Endl;
                        successfullyLoad = true;
                        break;
                    } else {
                        WARNING_LOG << "Configuration loaded on " << att + 1 << " attemption failed" << Endl;
                    }
                } catch (...) {
                    ERROR_LOG << "Exception occurred during loading of the configs: " << CurrentExceptionMessage() << Endl;
                }
            }
            if (!successfullyLoad) {
                ERROR_LOG << "Configuration was not updated" << Endl;
            }
        }

        RereadBaseConfig();

        if (!!EnvironmentManager) {
            TTimeGuard timeGuard("Init configuration");
            EnvironmentManager->ClearGlobal();
            if (EnvironmentManager->CheckCurrent() && EnvironmentManager->GetStatus() == NController::FailedConfig) {
                if (!EnvironmentManager->RestoreConfiguration(EnvironmentManager->GetLastSuccessConfiguration())) {
                    SetStatus(ssbcNotRunnable);
                    return false;
                }
                if (EnvironmentManager->CheckCurrent() && EnvironmentManager->GetStatus() == NController::FailedConfig) {
                    SetStatus(ssbcNotRunnable);
                    return false;
                }
                RereadBaseConfig();
            }
        }

        {
            THolder<TWriteGuard> wg;
            if (!ConfigParams.GetDaemonConfig()->GetController().StartServer) {
                INFO_LOG << "Server has not been started" << Endl;
                wg.Reset(new TWriteGuard(ConfigMutex));
                Config.Drop();
                return false;
            } else {
                INFO_LOG << "Server starting..." << Endl;
                AssertCorrectConfig(ConfigParams.TextIsCorrect(), "Config text processing failed");
                try {
                    NController::TStatusGuard sg(EnvironmentManager.Get(), NController::FailedConfig);
                    TAtomicSharedPtr<IServerConfig> newConfig(Descriptor.CreateConfig(ConfigParams));
                    CheckConfig(*newConfig, false);
                    wg.Reset(new TWriteGuard(ConfigMutex));
                    Config.Swap(newConfig);
                } catch (...) {
                    ERROR_LOG << "Server starting problems: " << CurrentExceptionMessage() << Endl;
                    wg.Reset(new TWriteGuard(ConfigMutex));
                    Config.Drop();
                    const TString str = CurrentExceptionMessage();
                    AbortFromCorruptedConfig("Config problems: %s", str.data());
                    return false;
                }
            }
        }

        return true;
    };

    void TController::ClearConfigurations() {
        if (EnvironmentManager) {
            EnvironmentManager->ClearConfigurations();
        }
    }

    void TController::ClearServerDataStatus() {
        if (!!EnvironmentManager) {
            EnvironmentManager->Clear();
        }
    }

    bool TController::RestartServer(const TRestartContext& ctx) {
        TGuard<TMutex> gRestart(RestartMutex);
        try {
            {
                TWriteGuard g(ServerMutex);
                DestroyServerImpl(TDestroyServerContext(ctx));
            }
            if (InitConfiguration(ctx)) {
                if (!!EnvironmentManager) {
                    NController::TSlotStatus slotStatus = EnvironmentManager->GetStatus();
                    if (slotStatus == NController::FailedIndex || (slotStatus == NController::UnknownError && EnvironmentManager->IncrementRestartAttemptions("") > GetDaemonConfig()->GetController().GetMaxRestartAttemptions())) {
                        WARNING_LOG << "Server is not runnable current configuration: " << NController::TSlotStatus_Name(slotStatus) << Endl;
                        SetStatus(NController::ssbcNotRunnable);
                        return false;
                    }
                }
                TWriteGuard g(ServerMutex);
                if (!!EnvironmentManager) {
                    EnvironmentManager->SetStatus("", NController::UnknownError);
                }
                if (StartServerImpl(ctx)) {
                    if (!!EnvironmentManager) {
                        EnvironmentManager->Success();
                        return true;
                    }
                }
            }
        } catch (...) {
            S_FAIL_LOG << "Unexpected exception: " << CurrentExceptionMessage() << Endl;
        }

        return false;
    }

    void TController::DestroyServer(const TDestroyServerContext& ctx) {
        TWriteGuard g(ServerMutex);
        DestroyServerImpl(ctx);
    }

    void TController::DestroyServerImpl(const TDestroyServerContext& ctx) noexcept {
        TTimeGuard stopDropGuard(ctx.MutableStopTimePtr(), TString("Server ") + (!!Server ? "exists. Stop and null server" : "not exists"));
        if (!!Server) {
            try {
                SetStatus(NController::ssbcStopping);
                {
                    TTimeGuard stopGuard("Stop server");
                    if (!!ctx.GetSleepTimeout()) {
                        TTimeGuard sleepGuard("Sleep");
                        Sleep(ctx.GetSleepTimeout());
                    }
                    Server->Stop(ctx.GetRigidStopLevel());
                }
                Server.Drop();
                SetStatus(NController::ssbcStopped);
            } catch (...) {
                S_FAIL_LOG << "Unexpected exception: " << CurrentExceptionMessage() << Endl;
            }
        }
    }

    void TController::Stop(ui32 rigidStopLevel) {
        if (ConfigParams.GetPreprocessor()->ItsConfigPathExists()) {
            StopItsWatcher();
        }
        DestroyServer(TDestroyServerContext(rigidStopLevel));
        if (GetDaemonConfig()->GetController().Enabled) {
            try {
                TGuard<TMutex> guard(ControllerMutex);
                THttpServer::Shutdown();
                Wait();
                AsyncExecuter.Stop();
            } catch (...) {
                ERROR_LOG << "Error while TController::Stop " << CurrentExceptionMessage() << Endl;
            }
        }
        Stopped.Signal();
    }

    void TController::Run() {
        ThreadDisableBalloc();
        if (ConfigParams.GetDaemonConfig()->GetController().Enabled) {
            TGuard<TMutex> guard(ControllerMutex);
            AsyncExecuter.Start();
            VERIFY_WITH_LOG(Start(), "cannot start controller http server: %s", GetError());
        }
        RestartServer(NController::TRestartContext(0));
        if (ConfigParams.GetDaemonConfig()->GetController().AutoStop) {
            Stop(0);
        }
        if (ConfigParams.GetPreprocessor()->ItsConfigPathExists()) {
            StartItsWatcher();
        }
    }

    TController::TController(TServerConfigConstructorParams& params, const IServerDescriptor& descriptor)
        : THttpServer(this, params.GetDaemonConfig()->GetController())
        , ConfigParams(params)
        , Log(MakeAtomicShared<TLog>(params.GetDaemonConfig()->GetController().Log))
        , Status(NController::ssbcStopped)
        , ControllerStart(Now())
        , AsyncExecuter(params.GetDaemonConfig()->GetController().nThreads)
        , Descriptor(descriptor)
    {
        RereadBaseConfig();
        const TDuration liveTime = params.GetDaemonConfig()->GetLiveTime();
        if (liveTime != TDuration::Zero()) {
            LiveTimeBomb.Reset(new TBomb(liveTime, "Live time exceeded"));
        }
        RegisterGlobalMessageProcessor(this);
    }

    TController::~TController() {
        DestroyServer(TDestroyServerContext(1));
        UnregisterGlobalMessageProcessor(this);
    }

    TController::TClient::TClient(TController& owner)
        : Owner(owner)
    {
    }

    bool TController::TClient::Reply(void* /*ThreadSpecificResource*/) {
        ThreadDisableBalloc();
        try {
            ProcessHeaders();
            RD.Scan();
            LogRequest();
            if (ProcessSpecialRequest()) {
                return true;
            }

            const TString& command = RD.CgiParam.Get("command");
            TController::TCommandProcessor::TPtr worker(Owner.Descriptor.CreateCommandProcessor(command));
            if (!worker) {
                Output() << "HTTP/1.1 501 Not Implemented\r\n\r\n";
                Output() << "Unknown command " << command;
                return true;
            }

            bool async = RD.CgiParam.Has("async") && FromString<bool>(RD.CgiParam.Get("async"));
            worker->Init(Owner, this, async);
            if (async) {
                NJson::TJsonValue result;
                Owner.ExecuteAsyncCommand(worker.Release(), result);
                if (PostBuffer().Size())
                    result.InsertValue("content_hash", MD5::Calc(TStringBuf(PostBuffer().AsCharPtr(), PostBuffer().Size())));
                TString reply = NJson::WriteJson(result, true, true, false);
                Output() << "HTTP/1.1 202 Data Accepted\r\n"
                    "Content-Length:" << reply.size() << "\r\n"
                    "\r\n"
                    << reply;
                return true;
            } else {
                worker->Process(nullptr);
                return true;
            }
        } catch (...) {
            ERROR_LOG << "Error while TController::TClient::Reply " << CurrentExceptionMessage() << Endl;
            try {
                Output() << "HTTP/1.1 500 Internal Server Error\r\n\r\n" << CurrentExceptionMessage();
            } catch (...) {
            } // suppress error if the socket is broken
        }
        return true;
    }

    void TController::TClient::ProcessTass(IOutputStream& out) const {
        if (Owner.GetStatus() == NController::ssbcActive) {
            SendGlobalMessage<TMessageUpdateUnistatSignals>();
        }
        TBaseHttpClient::ProcessTass(out);
    }

    void TController::TClient::LogRequest() {
        if (IsTrue(RD.CgiParam.Get("tire_req"))) {
            return;
        }

        NTskvFormat::TLogBuilder record("controller-incoming");
        const TInstant now = Now();
        record.Add("ts", ToString(now.Seconds()));
        record.Add("time", now.ToString());
        record.Add("reqstamp", ToString(RD.RequestBeginTime()));
        record.Add("ip", RD.RemoteAddr());
        record.Add("script", RD.ScriptName());
        record.Add("params", RD.Query());
        record.Add("post-size", ToString(PostBuffer().Size()));

        if (auto log = Owner.GetLog()) {
            (*log) << record.Str() << Endl;
        }
    }

    //TController::TCommandProcessor
    void TController::TCommandProcessor::Init(TController& owner, TController::TClient* requester, bool async) {
        Async = async;
        Owner = &owner;
        if (Async) {
            Requester = nullptr;
            Cgi = requester->GetRD().CgiParam;
            Buf.Assign(requester->PostBuffer().AsCharPtr(), requester->PostBuffer().Size());
        } else {
            Requester = requester;
        }
    }

    void TController::TCommandProcessor::Process(void* /*ts*/) {
        ThreadDisableBalloc();
        SetStatus(stsStarted);
        TStringBuf data = GetPostBuffer();
        if (!Async && !!data)
            Write("content_hash", MD5::Calc(data));
        Write("command", GetName());
        const TString timeoutStr = GetCgi().Get("timeout");
        TDuration timeout;
        if (!!timeoutStr)
            timeout = TDuration::Parse(timeoutStr);
        TBomb bomb(timeout, "process commmand " + GetName() + " too long");
        bool success = true;
        try {
            DoProcess();
            SetStatus(stsFinished);
        } catch (const yexception& e) {
            Write("error", e.what());
            ERROR_LOG << "Error while processing command " << GetName() << ": " << e.what() << Endl;
            success = false;
            SetStatus(stsFailed, e.what());
        }
        if (!Async) {
            TString replyStr = NJson::WriteJson(*GetReply(), true, true, false);
            TStringStream ss;
            ss << "HTTP/1.1 " << (success ? 200 : 400) << " Ok\r\nContent-Type: text/json\r\nContent-Length: " << replyStr.size() << "\r\nAccess-Control-Allow-Origin: *"
                << "\r\n\r\n"
                << replyStr;
            Requester->Output().Write(ss.Str());
            Requester->Output().Finish();
        };
    }

    TController::TGuardServer TController::TCommandProcessor::GetServer(bool mustBe) {
        TController::TGuardServer server = Owner->GetServer();
        if (mustBe && !server)
            ythrow yexception() << "No server";
        return server;
    }

    ui32 TController::TCommandProcessor::ParseRigidStopLevel(const TCgiParameters& params, ui32 fallback) const {
        if (params.Find("rigid_level") != params.end()) {
            return FromString<ui32>(params.Get("rigid_level"));
        }
        if (params.Find("rigid") != params.end() && FromString<bool>(params.Get("rigid"))) {
            return Max<ui32>();
        }
        return fallback;
    }
}
