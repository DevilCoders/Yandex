#include "server.h"

#include "admin.h"
#include "bad_request_handler.h"
#include "blocked_handler.h"
#include "blocker.h"
#include "captcha_check.h"
#include "captcha_key.h"
#include "captcha_req_replier.h"
#include "cl_params.h"
#include "conditional_handler.h"
#include "config_global.h"
#include "dynamic_thread_pool.h"
#include "environment.h"
#include "eventlog_err.h"
#include "fullreq_handler.h"
#include "fullreq_info.h"
#include "get_host_name_request.h"
#include "its_files_watcher.h"
#include "req_replier.h"
#include "reqinfo_request.h"
#include "request_context.h"
#include "request_time_stats.h"
#include "selective_handler.h"
#include "server_error_handler.h"
#include "static_handler.h"
#include "unified_agent_log.h"
#include "user_reply.h"

#include <antirobot/captcha/localized_data.h>
#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/lib/antirobot_response.h>
#include <antirobot/lib/spravka.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/neh/http2.h>
#include <library/cpp/sighandler/async_signals_handler.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/guid.h>
#include <util/generic/xrange.h>
#include <util/string/type.h>
#include <util/string/vector.h>
#include <util/system/event.h>
#include <util/system/thread.h>

#include <utility>

namespace NAntiRobot {

    namespace {

    bool IsEnableServerError(TRequestContext& rc) {
        return AtomicGet(rc.Env.ServerErrorFlags.Enable) && !AtomicGet(rc.Env.ServerErrorFlags.ServiceDisable.GetByService(rc.Req->HostType));
    }

    bool GetBanSourceIp(const TRequestContext& rc) {
        auto request = rc.Req;
        if (request->UserAddr.IsWhitelisted()) {
            return false;
        }
        return !rc.MatchedRules->BanSourceIp.empty();
    }

    bool GetBanFWSourceIp(const TRequestContext& rc) {
        auto request = rc.Req;
        if (request->UserAddr.IsWhitelisted()) {
            return false;
        }
        return !rc.MatchedRules->BanFWSourceIp.empty();
    }

    bool IsBlocked(TRequestContext& rc) {
        TIsBlocked::TBlockState blockState = rc.Env.IsBlocked.GetByService(rc.Req->HostType)->GetBlockState(rc);
        rc.Degradation = GetDegradation(rc);

        if (rc.MatchedRules->Mark) {
            rc.Env.BlockResponsesStats->UpdateMarked(rc);
        }

        if (rc.MatchedRules->UserMark) {
            rc.Env.BlockResponsesStats->UpdateUserMarked(*rc.Req);
        }

        rc.BanSourceIp = GetBanSourceIp(rc);
        if (rc.BanSourceIp) {
            rc.Env.ServiceExpBinCounters.Inc(*rc.Req, TEnv::EServiceExpBinCounter::BanSourceIp);
        }

        rc.BanFWSourceIp = GetBanFWSourceIp(rc);
        if (rc.BanFWSourceIp) {
            rc.Env.ServiceExpBinCounters.Inc(*rc.Req, TEnv::EServiceExpBinCounter::BanFWSourceIp);
            rc.Env.BanFWAddrs.Enqueue(rc.Req->RawAddr);
        }

        if (blockState.IsBlocked) {
            if (rc.Env.DisablingFlags.IsNeverBlockForService(rc.Req.Get()->HostType)) {
                rc.Env.DisablingStat.AddNotBlockedRequest();
                return false;
            }
            rc.Env.BlockResponsesStats->Update(rc);
            rc.WasBlocked = true;
            rc.BlockReason = blockState.Reason;
            return true;
        }
        return false;
    }

    bool NeedCutMessengerUserAgent(const TRequestContext& rc) {
        const auto& req = *rc.Req;
        return req.PreviewAgentType() != EPreviewAgentType::UNKNOWN && req.Uid.Ns != TUid::PREVIEW;
    }

    void AddGetRequestsToRouter(TSelectiveHandler<THttpMethodSelector>& httpMethodSelector) {
        httpMethodSelector.Add("GET", HandleUrlLocation()
            .Add("/ver", HandleVer)
            .Add("/ping", HandlePing)
            .Add("/admin", HandleCgi(ADMIN_ACTION)
                .Add("memstats",     HandleMemStats)
                .Add("blockstats",   HandleBlockStats)
                .Add("unistats",     HandleUniStats)
                .Add("unistats_lw",  HandleUniStatsLW)
                .Add("dumpcfg",      HandleDumpCfg)
                .Add("loglevel",     HandleLogLevel)
                .Add("getspravka",   HandleGetSpravka)
                .Add("dumpcache",    HandleDumpCache)
                .Default(FromLocalHostOnly(HandleCgi(ADMIN_ACTION)
                    .Add("shutdown",          HandleShutdown)
                    .Add("reloaddata",        HandleReloadData)
                    .Add("reloadlkeys",       HandleReloadLKeys)
                    .Add("amnesty",           HandleAmnesty)
                    .Add("workmode",          HandleWorkMode)
                    .Add("block",             HandleForceBlock)
                    .Add("get_cbb_rules", HandleGetCbbRules)
                    .Add("start_china_redirect", HandleStartChinaRedirect)
                    .Add("stop_china_redirect", HandleStopChinaRedirect)
                    .Add("start_china_unauthorized", HandleStartChinaUnauthorized)
                    .Add("stop_china_unauthorized", HandleStopChinaUnauthorized)
                    .Default(HandleUnknownAdminAction)
                ))
            )
        );
    }

    void AddGetUnistatRequestsToRouter(TSelectiveHandler<THttpMethodSelector>& httpMethodSelector) {
        httpMethodSelector.Add("GET", HandleUrlLocation()
            .Add("/unistats",     HandleUniStats)
            .Add("/unistats_lw",     HandleUniStatsLW)
            );
    }

    std::function<NThreading::TFuture<TResponse>(TRequestContext&)> CreateRequestRouter() {
        auto ret = HandleHttpMethod();
        ret.Add("POST", HandleUrlLocation()
                    .Add("/fullreq", TFullreqHandler(
                        HandleUrlLocation()
                        .Add(COMMAND_GET_HOST_NAME, HandleGetHostNameRequest)
                        .Add(COMMAND_GETREQINFO, HandleReqInfoRequest)
                        .Add("/captchapage", TestCaptchaPage)
                        .Add("/blockedpage", TestBlockedPage)
                        .Add("/manyrequestspage", TestManyRequestsPage)
                        .Default(ConditionalHandler(IsEnableServerError)
                            .IfTrue(TServerErrorHandler())
                            .IfFalse(HandleUrlLocation()
                                    .Default(ConditionalHandler(IsBlocked)
                                            .IfTrue(TBlockedHandler())
                                            .IfFalse(HandleUrlLocation()
                                                    .Add("/showcaptcha",        HandleShowCaptcha)
                                                    .Add("/captchapgrd",        HandleCaptchaPgrd)
                                                    .Add("/tmgrdfrendpgrd",     HandleCaptchaPgrd)
                                                    .Add("/captchaimg",         HandleShowCaptchaImage)
                                                    .Add("/captcha/voice",      HandleVoice)
                                                    .Add("/captcha/voiceintro", HandleVoiceIntro)
                                                    .Add("/checkcaptcha",       HandleCheckCaptcha)
                                                    .Add("/xcheckcaptcha",      HandleCheckCaptcha)
                                                    .Add("/checkcaptchajson",   HandleCheckCaptcha)
                                                    .Add("/tmgrdfrendc",        HandleCheckCaptcha)
                                                    .Add("/xmlsearch",          HandleXmlSearch)
                                                    .Add("/search/xml",         HandleXmlSearch)
                                                    .Add(TStaticData::Instance().GetHandler())
                                                    .Default(ConditionalHandler(NeedCutMessengerUserAgent)
                                                            .IfTrue(HandleMessengerRequest)
                                                            .IfFalse(HandleGeneralRequest)
                                                    ))
                                        ))))));
        AddGetRequestsToRouter(ret);
        return ret;
    }

    THttpServerOptions CreateServerOptions(ui16 port,
                                           TDuration readRequestTimeout,
                                           size_t maxConnections,
                                           const TAntirobotDaemonConfig::TMtpQueueParams& queueParams)
    {
        THttpServer::TOptions options(port);
        options.SetThreads(queueParams.ThreadsMax)
               .SetMaxConnections(maxConnections)
               .SetClientTimeout(readRequestTimeout)
               .SetThreads(queueParams.ThreadsMin)
               .SetMaxQueueSize(queueParams.QueueSize);
        return options;
    }

    TSimpleSharedPtr<TPreemptiveMtpQueue> CreateServerQueue(const TAntirobotDaemonConfig::TMtpQueueParams& params) {
        return new TPreemptiveMtpQueue(GetAntiRobotDynamicThreadPool(), params.ThreadsMax);
    }

    TSimpleSharedPtr<THttpServer> CreateServer(
        THttpServer::ICallBack& callback,
        const THttpServerOptions& options,
        THttpServer::TMtpQueueRef queue
    ) {
        THttpServer::TMtpQueueRef failWorkers(new TThreadPool(GetAntiRobotDynamicThreadPool()));
        return new THttpServer(&callback, std::move(queue), failWorkers, options);
    }

    TStatPrinter CreateServerStatPrinter(
        TString serverName,
        TSimpleSharedPtr<TPreemptiveMtpQueue> serverQueue,
        TTimeStats& fullTime, TTimeStats& handleTime,
        TTimeStats& readTime, TTimeStats& waitTime
    ) {
        return [
            serverName = std::move(serverName),
            serverQueue = std::move(serverQueue),
            &fullTime,
            &handleTime,
            &readTime,
            &waitTime
        ] (TStatsWriter& out) {
            auto prefixedOut = out.WithPrefix(serverName + ".");
            serverQueue->PrintStatistics(prefixedOut);
            fullTime.PrintStats(prefixedOut);
            handleTime.PrintStats(prefixedOut);
            readTime.PrintStats(prefixedOut);
            waitTime.PrintStats(prefixedOut);
        };
    }

    }

    class TServer::TImpl : public TEventHandler {
    public:
        TImpl(const TCommandLineParams& clParams)
            : ClParams(clParams)
        {
        };

        void Run() {

            InitNetworkSubSystem();
            {
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.SetBaseDir(ClParams.BaseDirName);
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.SetLogsDir(ClParams.LogsDirName);
                ANTIROBOT_CONFIG_MUTABLE.LoadFromPath(ClParams.ConfigFileName);
            }

            NNeh::SetHttp2OutputConnectionsLimits(ANTIROBOT_DAEMON_CONFIG.NehOutputConnectionsSoftLimit,
                                                  ANTIROBOT_DAEMON_CONFIG.NehOutputConnectionsHardLimit);

            ANTIROBOT_DAEMON_CONFIG_MUTABLE.DebugOutput = ClParams.DebugMode;
            if (ClParams.Port != 0) {
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.Port = ClParams.Port;
            }
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.UseTVMClient = !ClParams.NoTVMClient;
            ANTIROBOT_DAEMON_CONFIG.Dump(Cerr, HOST_WEB);

            auto env = MakeHolder<TEnv>();

            const auto& serverQueueParams = ANTIROBOT_DAEMON_CONFIG.ServerQueueParams;
            auto options = CreateServerOptions(ANTIROBOT_DAEMON_CONFIG.Port,
                                               ANTIROBOT_DAEMON_CONFIG.ServerReadRequestTimeout,
                                               ANTIROBOT_DAEMON_CONFIG.MaxConnections,
                                               serverQueueParams);
            auto serverQueue = CreateServerQueue(serverQueueParams);

            TCaptchaApiHandler captchaApiHandler(*env);
            auto captchaApiServer = CreateServer(captchaApiHandler, options, serverQueue);

            TCacherHandler cacherHandler(*env);
            auto cacherServer = CreateServer(cacherHandler, options, serverQueue);

            TProcessorHandler processHandler(*env);
            const auto& processorQueueParams = ANTIROBOT_DAEMON_CONFIG.ProcessServerQueueParams;
            auto processorOptions = CreateServerOptions(ANTIROBOT_DAEMON_CONFIG.ProcessServerPort,
                                                        ANTIROBOT_DAEMON_CONFIG.ProcessServerReadRequestTimeout,
                                                        ANTIROBOT_DAEMON_CONFIG.ProcessServerMaxConnections,
                                                        processorQueueParams);
            auto processQueue = CreateServerQueue(processorQueueParams);
            auto processorServer = CreateServer(processHandler, processorOptions, processQueue);

            if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
                Servers.push_back(captchaApiServer);

                env->AddStatPrinter([captchaApiServer](TStatsWriter& out) {
                    out.WithPrefix("http_server.").WriteHistogram("active_connections", captchaApiServer->GetClientCount());
                });
            } else {
                Servers.push_back(cacherServer);
                Servers.push_back(processorServer);

                env->AddStatPrinter(CreateServerStatPrinter("process_server", processQueue,
                                                            env->ProcessServerTimeStats,
                                                            env->ProcessServerTimeStatsHandle,
                                                            env->ProcessServerTimeStatsRead,
                                                            env->ProcessServerTimeStatsWait));

                env->AddStatPrinter([cacherServer, processorServer](TStatsWriter& out) {
                    out.WithPrefix("http_server.").WriteHistogram("active_connections", cacherServer->GetClientCount());
                    out.WithPrefix("process_server.").WriteHistogram("active_connections", processorServer->GetClientCount());
                });
            }

            TAdminHandler adminHandler(*env);
            THttpServer::TOptions adminOptions(ANTIROBOT_DAEMON_CONFIG.AdminServerPort);
            adminOptions.SetThreads(ANTIROBOT_DAEMON_CONFIG.AdminServerThreads)
                        .SetMaxConnections(ANTIROBOT_DAEMON_CONFIG.AdminServerMaxConnections)
                        .SetClientTimeout(ANTIROBOT_DAEMON_CONFIG.AdminServerReadRequestTimeout)
                        .SetMaxQueueSize(serverQueueParams.QueueSize);
            Servers.push_back(MakeSimpleShared<THttpServer>(&adminHandler, adminOptions));

            TUnistatHandler unistatHandler(*env);
            THttpServer::TOptions unistatOptions(ANTIROBOT_DAEMON_CONFIG.UnistatServerPort);
            unistatOptions.SetThreads(1)
                        .SetMaxConnections(ANTIROBOT_DAEMON_CONFIG.UnistatServerMaxConnections)
                        .SetClientTimeout(ANTIROBOT_DAEMON_CONFIG.UnistatServerReadRequestTimeout)
                        .SetMaxQueueSize(serverQueueParams.QueueSize);
            Servers.push_back(MakeSimpleShared<THttpServer>(&unistatHandler, unistatOptions));

            for (auto i : xrange(Servers.size())) {
                if (!Servers[i]->Start()) {
                    ythrow yexception() << "Server " << i << ": " << Servers[i]->GetError();
                }
                env->Servers.push_back(Servers[i]);
            }

            env->AddStatPrinter(CreateServerStatPrinter("http_server", serverQueue,
                                                        env->TimeStats,
                                                        env->TimeStatsHandle,
                                                        env->TimeStatsRead,
                                                        env->TimeStatsWait));

            SetAsyncSignalHandler(SIGTERM, TAutoPtr<TEventHandler>(this));

            TItsFilesWatcher itsFilesWatcher(
                env->DisablingFlags,
                env->PanicFlags,
                env->DisablingStat,
                env->AmnestyFlags,
                env->ServerErrorFlags,
                env->SuspiciousFlags,
                env->AntirobotDisableExperimentsFlag,
                env->RSWeight
            );

            ANTIROBOT_DAEMON_CONFIG.DumpNumUnresolvedDaemons(Cerr);
            Cerr << "[" << TInstant::Now() << " (UTC)] " << "Server has started on port " << ANTIROBOT_DAEMON_CONFIG.Port << Endl;

            for (auto& server : Servers) {
                server->Wait();
            }

        }

        // Handles SIGTERM signal
        int Handle(int) override {
            for (auto& server : Servers) {
                if (server) {
                    server->Shutdown();
                }
             }
             return 0;
        }
    private:
        struct TCacherHandler: public THttpServer::ICallBack {
            TCacherHandler(TEnv& env)
                : Env(env)
            {
                Handler = CreateRequestRouter();
            }

            TClientRequest* CreateClient() override {
                const TRequestTimeStats requestTimeStats = {Env.TimeStats, Env.TimeStatsWait, Env.TimeStatsRead,
                                                            Env.TimeStatsCaptcha};
                return new TReqReplier(Env, Handler, requestTimeStats,
                                       ANTIROBOT_DAEMON_CONFIG.ServerFailOnReadRequestTimeout);
            }

            TEnv& Env;
            std::function<NThreading::TFuture<TResponse>(TRequestContext&)> Handler;
        };

        struct TProcessorHandler: public THttpServer::ICallBack {
            TProcessorHandler(TEnv& env)
                : Env(env)
                , Handler(HandleHttpMethod()
                          .Add("POST", HandleUrlLocation()
                                       .Add(ANTIROBOT_DAEMON_CONFIG.RequestForwardLocation, HandleProcessRequest)
                                       .Add(ANTIROBOT_DAEMON_CONFIG.CaptchaInputForwardLocation, HandleProcessCaptchaInput)))
            {
            }

            TClientRequest* CreateClient() override {
                const TRequestTimeStats requestTimeStats = {Env.ProcessServerTimeStats, Env.ProcessServerTimeStatsWait,
                                                            Env.ProcessServerTimeStatsRead};
                return new TReqReplier(Env, Handler, requestTimeStats, true);
            }

            TEnv& Env;
            std::function<NThreading::TFuture<TResponse>(TRequestContext&)> Handler;
        };

        struct TAdminHandler: public THttpServer::ICallBack {
            TAdminHandler(TEnv& env)
                : Env(env)
            {
                auto tempHandler = HandleHttpMethod();
                AddGetRequestsToRouter(tempHandler);
                Handler = tempHandler;
            }

            TClientRequest* CreateClient() override {
                return new TReqReplier(Env, Handler, {Env.TimeStats, Env.TimeStatsWait}, true);
            }

            TEnv& Env;
            std::function<NThreading::TFuture<TResponse>(TRequestContext&)> Handler;
        };

        struct TUnistatHandler: public THttpServer::ICallBack {
            explicit TUnistatHandler(TEnv& env)
                : Env(env)
            {
                auto tempHandler = HandleHttpMethod();
                AddGetUnistatRequestsToRouter(tempHandler);
                Handler = tempHandler;
            }

            TClientRequest* CreateClient() override {
                return new TReqReplier(Env, Handler, {Env.TimeStats, Env.TimeStatsWait}, true);
            }

            TEnv& Env;
            std::function<NThreading::TFuture<TResponse>(TRequestContext&)> Handler;
        };

        struct TCaptchaApiHandler: public THttpServer::ICallBack {
            TCaptchaApiHandler(TEnv& env)
                : Env(env)
                , Handler()
            {
                TSelectiveHandler<TUrlLocationSelector> iframeHtmlsHandler;
                for (const TString& path : TLocalizedData::Instance().GetExternalVersionedFiles()) {
                    if (path.EndsWith(".html")) {
                        iframeHtmlsHandler.Add(path, HandleCaptchaIframeHtml);
                    }
                }

                Handler = HandleHttpMethod()
                            .Add("POST", HandleUrlLocation()
                                .Add("/check",       HandleCheckCaptchaApi)
                                .Add("/tmgrdfrendc", HandleCheckCaptcha)
                                .Add("/demo",        HandleCaptchaDemo)
                            )
                            .Add("GET", HandleUrlLocation()
                                .Add(TStaticData::Instance().GetHandler())
                                .Add("/validate",             HandleCaptchaValidate)
                                .Add("/captchaimg",           HandleShowCaptchaImage)
                                .Add("/captcha/voice",        HandleVoice)
                                .Add("/captcha/voiceintro",   HandleVoiceIntro)
                                .Add("/captchapgrd",          HandleCaptchaPgrd)
                                .Add("/tmgrdfrendpgrd",       HandleCaptchaPgrd)
                                .Add("/demo",                 HandleCaptchaDemo)
                                .Add(iframeHtmlsHandler)
                            );
            }

            TClientRequest* CreateClient() override {
                const TRequestTimeStats requestTimeStats = {Env.TimeStats, Env.TimeStatsWait, Env.TimeStatsRead,
                                                            Env.TimeStatsCaptcha};
                return new TCaptchaReqReplier(Env, Handler, requestTimeStats,
                                    ANTIROBOT_DAEMON_CONFIG.ServerFailOnReadRequestTimeout);
            }

            TEnv& Env;
            std::function<NThreading::TFuture<TResponse>(TRequestContext&)> Handler;
        };

        TVector<TSimpleSharedPtr<THttpServer>> Servers;
        const TCommandLineParams& ClParams;
    };

    TServer::TServer(const TCommandLineParams& clParams)
        : Impl(new TImpl(clParams))
    {
    }


    TServer::~TServer() {
    }


    void TServer::Run() {
        Impl->Run();
    }
}
