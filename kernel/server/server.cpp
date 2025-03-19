#include "server.h"

#include <google/protobuf/text_format.h>

#include <kernel/searchlog/errorlog.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/misc/httpdate.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/threading/equeue/equeue.h>

#include <util/datetime/base.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/stream/file.h>
#include <util/stream/null.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/system/tls.h>

namespace NServer {

    namespace {
        class TStatDumper {
        public:
            TStatDumper(TServer& server, const TRequest& request, const THttpResponse& response, const TInstant startTime)
                : Stats(server.GetStats())
                , Request(request)
                , Response(response)
                , Start(std::move(startTime))
            {
                Stats.HitQueue(
                    server.GetRequestQueueSize() + server.GetApphostQueueSize(), server.GetFailQueueSize(),
                    server.GetRequestQueueObjectCount(), server.GetFailQueueObjectCount());
            }

            void SetProductionTraffic(const bool value) {
                ProductionTraffic = value;
            }

            ~TStatDumper() {
                if (Request.NeedRegisterProcessingTime(ProductionTraffic, Response.HttpCode())) {
                    const auto replyTime = Now() - Start;
                    Stats.HitTime(replyTime.MicroSeconds() * 0.001, ProductionTraffic);
                }

                if (Request.NeedRegisterProcessingStatus(ProductionTraffic, Response.HttpCode())) {
                    Stats.HitRequest(Response.HttpCode());
                }
            }

        private:
            bool ProductionTraffic = false;
            TServerStats& Stats;
            const TRequest& Request;
            const THttpResponse& Response;
            const TInstant Start;
        };
    }

    class TDaemonResource {
    public:
        TDaemonResource() = default;

        ~TDaemonResource() {
            if (Resource && Master) {
                try {
                    Master->DestroyThreadSpecificResource(Resource);
                } catch (...) {
                }
            }
        }

        void Init(TServer* master) {
            Y_ENSURE(master);
            Master = master;
            Resource = Master->CreateThreadSpecificResource();
        }

        void* Get() {
            Y_ENSURE(Resource && Master);
            return Resource;
        }

        operator bool() const {
            return Resource && Master;
        }

    private:
        TServer* Master = nullptr;
        void* Resource = nullptr;
    };

    const TServerRequestData& TRequest::RequestData() const {
        return RD;
    }

    TRequest::TRequest(TServer& parentWebdaemon)
        : ParentWebdaemon(parentWebdaemon)
    {
    }

    bool TRequest::IsTimeOutRequest() const {
        Y_ASSERT(ServerResource != nullptr);
        Y_ENSURE(ServerResource != nullptr, "[TRequest] server resource not set at begin");

        TDuration waitTime = TInstant::Now() - CreateTime;
        return waitTime > TDuration::MilliSeconds(ServerResource->HttpConfig->GetRequestTTL());
    }

    // Default virtual methods implementation
    void TRequest::AssignResource(void* res) {
        Y_UNUSED(res);
        return;
    }

    const TString& TRequest::CacheControlHeaderString() const {
        return Default<TString>();
    }

    bool TRequest::DoReply(const TString& scriptName, THttpResponse& response) {
        Y_UNUSED(scriptName);
        Y_UNUSED(response);
        return false;
    }

    bool TRequest::HandleAdmin(const TString& action, THttpResponse& response) {
        Y_UNUSED(action);
        Y_UNUSED(response);
        return false;
    }

    bool TRequest::ProcessHeaders() {
        return true;
    }

    // Base methods implementation
    void TRequest::BaseAssignResource(void* res) {
        if (res) {
            ServerResource = reinterpret_cast<TThreadResource*>(res);
            UpdateSharedItsConfigBlob(ServerResource->ItsConfigBlob);
        }
    }

    THttpResponse TRequest::BaseHandleAdmin() {
        if (!ParentWebdaemon.IsRun()) { // This is a special case for fuzzing
            return TextResponse("Service is not running yet", HTTP_BAD_REQUEST);
        }

        if (!IsLocal()) {
            return TextResponse("Permission denied", HTTP_FORBIDDEN);
        }

        const auto actionPtr = MapFindPtr(RD.CgiParam, "action");
        if (!actionPtr) {
            return TextResponse("cgi param action not found", HTTP_BAD_REQUEST);
        }

        THttpResponse response;
        if (*actionPtr == "shutdown") {
            return AdminShutdown();
        } else if (*actionPtr == "reopen_logs") {
            return AdminReopenLogs();
        } else if (HandleAdmin(*actionPtr, response)) {
            return response;
        }

        return TextResponse("unknown action", HTTP_BAD_REQUEST);
    }

    void TRequest::BaseProcessMethod() {
        const size_t pos = RequestString.find_first_of(' ');
        if (pos == TString::npos) {
            return;
        }

        Method = RequestString.substr(0, pos);
        Method.to_upper();

        if (Method == "HEAD") {
            Method = "GET";
            RequestString.replace(0, pos, Method);
            HeadRequest = true;
        }
    }

    bool TRequest::BaseProcessHeaders() {
        if (!THttpClientRequestEx::ProcessHeaders()) {
            return false;
        }

        RD.Scan();

        const auto* ims = RD.HeaderIn("If-Modified-Since");
        if (ims) {
            ModifiedSince = TInstant::MicroSeconds(parse_http_date(*ims));
        }
        KeyFromETag = StripString(RD.HeaderInOrEmpty("If-None-Match"), EqualsStripAdapter('\"'));
        RequestDate = FormatHttpDate(TInstant::MicroSeconds(RD.RequestBeginTime()).TimeT());

        return true;
    }

    THttpResponse TRequest::AdminShutdown() const {
        ParentWebdaemon.Shutdown();
        SEARCH_INFO << "shutting down";
        return TextResponse("shutting down\n");
    }

    THttpResponse TRequest::AdminReopenLogs() const {
        SEARCH_INFO << "reopening logs";
        ParentWebdaemon.ReopenLogs();
        SEARCH_INFO << "reopened logs";
        return TextResponse("logs are reopened\n");
    }

    THttpResponse TRequest::HandleHealth() const {
        return TextResponse("");
    }

    THttpResponse TRequest::HandleMstat() const {
        const auto fmtPtr = MapFindPtr(RD.CgiParam, "fmt");
        TServerStats::EReportFormat format = TServerStats::EReportFormat::Json;
        if (fmtPtr && *fmtPtr == "proto") {
            const auto hrPtr = MapFindPtr(RD.CgiParam, "hr");
            if (!hrPtr) {
                format = TServerStats::EReportFormat::Proto;
            } else if (*hrPtr == "da") {
                format = TServerStats::EReportFormat::ProtoHr;
            } else if (*hrPtr == "json") {
                format = TServerStats::EReportFormat::ProtoJson;
            }
        }

        int level = 0;
        const auto levelPtr = MapFindPtr(RD.CgiParam, "level");
        if (levelPtr) {
            if (!TryFromString(*levelPtr, level)) {
                ythrow yexception() << "Invalid value to level param " << *levelPtr;
            }
        }

        const auto& stats = ParentWebdaemon.GetStats();
        TStringStream out;
        stats.ReportMonitoringStat(out, format, level);

        THttpResponse response(HTTP_OK);
        response.SetContent(out.Str());

        return response;
    }

    THttpResponse TRequest::HandleRemoteAdmin() {
        const auto actionPtr = MapFindPtr(RD.CgiParam, "action");
        if (!actionPtr) {
            return TextResponse("cgi param action not found", HTTP_BAD_REQUEST);
        }

        if (*actionPtr == "health") {
            return HandleHealth();
        } else if (*actionPtr == "mstat") {
            return HandleMstat();
        }

        return TextResponse("unknown action", HTTP_BAD_REQUEST);
    }

    THttpResponse TRequest::HandleRobotsTxt() {
        return TextResponse("User-Agent: *\nDisallow: /\n");
    }

    THttpResponse TRequest::HandleSvnRevision() const {
        return TextResponse(GetProgramSvnVersion());
    }

    THttpResponse TRequest::TextResponse(TStringBuf content, HttpCodes httpCode) const {
        THttpResponse resp(httpCode);
        resp.AddHeader("Date", GetRequestDate());
        resp.AddHeader("Timing-Allow-Origin", "*");

        if (httpCode % 100 != 4 && httpCode % 100 != 5 && CacheControlHeaderString()) {
            resp.AddHeader("Cache-Control", CacheControlHeaderString());
        }

        if (IsHeadRequest()) {
            resp.AddHeader("Content-Length", content.size());
        } else {
            resp.SetContent(TString(content));
        }

        resp.SetContentType("text/plain");
        return resp;
    }

    THttpResponse TRequest::HandleScript() {
        THttpResponse response;
        TStatDumper dumper(ParentWebdaemon, *this, response, CreateTime);

        TStringBuf script = RD.ScriptName();
        if (script == "/admin") {
            response = BaseHandleAdmin();
        } else if (script == "/remote_admin") {
            response = HandleRemoteAdmin();
        } else if (script == "/svnrevision") {
            response = HandleSvnRevision();
        } else if (script == "/robots.txt") {
            response = HandleRobotsTxt();
        } else if (IsTimeOutRequest()) {
            response = TextResponse("too busy, request time out", HTTP_SERVICE_UNAVAILABLE);
        } else {
            dumper.SetProductionTraffic(true);

            bool processed = false;
            try {
                processed = DoReply(TString{script}, response);
            } catch (...) {
                response.SetHttpCode(HTTP_INTERNAL_SERVER_ERROR);
                throw;
            }

            if (!processed) {
                SEARCH_DEBUG << "[TRequest] script " << script << " not implemented";
                response = TextResponse("Not implemented", HTTP_NOT_FOUND);
            }
        }

        return response;
    }

    void TRequest::LoadItsConfig(const TBlob& configBlob) {
        Y_UNUSED(configBlob);
        return;
    }

    void TRequest::Reply(const TString& query, void* res) {
        try {
            if (!RD.Parse(query.c_str())) {
                return;
            }
            RD.Scan();

            BaseAssignResource(res);
            AssignResource(res);
            HandleScript();
        } catch (...) {
            // ignore errors
        }
    }

    bool TRequest::Reply(void* res) {
        BaseProcessMethod();

        if (!BaseProcessHeaders()) {
            return true;
        }

        if (!ProcessHeaders()) {
            return true;
        }

        try {
            BaseAssignResource(res);
            AssignResource(res);
            LoadItsConfig(ServerResource->ItsConfigBlob->GetBlob());

            Output() << HandleScript();
        } catch (...) {
            SEARCH_ERROR << "[TRequest] exception: " << CurrentExceptionMessage();
            Output() << TextResponse("Exception, see logs", HTTP_INTERNAL_SERVER_ERROR);
        }

        Output().Flush();
        Input().ReadAll(Cnull);
        return true;
    }

    bool TRequest::NeedRegisterProcessingTime(const bool productionTraffic, const HttpCodes httpCode) const {
        Y_UNUSED(productionTraffic);
        Y_UNUSED(httpCode);
        return true;
    }

    bool TRequest::NeedRegisterProcessingStatus(const bool productionTraffic, const HttpCodes httpCode) const {
        Y_UNUSED(productionTraffic);
        Y_UNUSED(httpCode);
        return true;
    }

    TServer::TServer(const TString& configPath)
        : IsRunning(0)
    {
        const TString protoString = TUnbufferedFileInput(configPath).ReadAll();
        const bool parseResult = google::protobuf::TextFormat::ParseFromString(protoString, &Config);
        Y_ASSERT(parseResult);
        Y_ENSURE(parseResult, "[TServer] error parsing config file");
        DoInit();
    }

    TServer::TServer(const NServer::THttpServerConfig& config)
        : Config(config)
        , IsRunning(0)
    {
        DoInit();
    }

    void TServer::DoInit() {
        IsRunning = 0;
        if (Config.HasLogFile()) {
            GET_ERRORLOG.ResetBackend(MakeHolder<TFileLogBackend>(Config.GetLogFile()));
        }
        SharedHttpServerConfig.AtomicStore(new TRefCountHttpServerConfig(Config));

        if (Config.HasEnableAppHost() && Config.GetEnableAppHost()) {
            const ui32 port = Config.HasAppHostPort() ? Config.GetAppHostPort() : Config.GetPort() + 1;
            const ui32 threads = Config.HasAppHostThreads() ? Config.GetAppHostThreads() : Config.GetThreads();
            const ui32 grpcPort = Config.HasGrpcPort() ? Config.GetGrpcPort() : port + 1;
            EnableAppHost(port, threads, Config.GetUseGrpcForApphost(), grpcPort, Config.GetGrpcThreadCount());
        }
    }

    void TServer::EnableAppHost(ui32 port, ui32 threads) {
        EnableAppHost(port, threads, false, 0, 0);
    }

    void TServer::EnableAppHost(ui32 port, ui32 threads, bool grpcEnabled, ui32 grpcPort, ui32 grpcThreadCount) {
        IsAppHostEnabled = true;
        AppHostPort = port;
        AppHostThreads = threads;
        IsGrpcEnabled = grpcEnabled;
        GrpcPort = grpcPort;
        GrpcThreadCount = static_cast<size_t>(grpcThreadCount);
    }

    void TServer::ReplyApphost(NAppHost::IServiceContext& ctx) {
        try {
            THolder<IApphostRequest> clientApphostRequest = CreateApphostClient();
            clientApphostRequest->Reply(ctx, GetThreadLocalResource());
        } catch (const yexception& ex) {
            SEARCH_ERROR << "[TServer] apphost request exception: " << ex.what();
            throw;
        } catch (...) {
            SEARCH_ERROR << "[TServer] apphost request unknown exception";
            throw;
        }
    }

    void* TServer::GetThreadLocalResource() {
        Y_STATIC_THREAD(TDaemonResource)
        daemonResource;

        if (!(daemonResource.Get())) {
            daemonResource.Get().Init(this);
        }

        return daemonResource.Get().Get();
    }

    void TServer::Start() {
        InitHttpServerOptionsFromConfig();
        InitStatsFromConfig();

        HttpServer = MakeHolder<THttpServer>(this, HttpServerOptions);

        const bool success = HttpServer->Start();
        Y_ENSURE(success, HttpServer->GetError());

        if (IsAppHostEnabled) {
            Loop = CreateApphostLoop();
            Loop->ForkLoop(AppHostThreads);
            SEARCH_INFO << "[TServer] started AppHostServant, port " << AppHostPort
                        << ", " << AppHostThreads << " threads";
            if (IsGrpcEnabled) {
                SEARCH_INFO << "[TServer] grpc enabled, grpc port " << GrpcPort
                        << ", " << GrpcThreadCount << " grpc threads";
            } else {
                SEARCH_INFO << "[TServer] grpc disabled";
            }
        }

        OnStart();

        AtomicSet(IsRunning, 1);
        auto itsWorkerThreadGuard = ITSWorkerThread.Access();
        itsWorkerThreadGuard->Reset(SystemThreadFactory()->Run(new TITSWorker(Config.GetConfigCheckDelay(), this, Config.GetDynamicConfigPath())));
    }

    THolder<NAppHost::TLoop> TServer::CreateApphostLoop() {
        auto loop = MakeHolder<NAppHost::TLoop>();
        if (IsGrpcEnabled) {
            loop->EnableGrpc(GrpcPort);
            loop->SetGrpcThreadCount(GrpcThreadCount);
        }

        auto appService = [this](NAppHost::IServiceContext& ctx) {
            this->ReplyApphost(ctx);
        };

        loop->Add(AppHostPort, appService);
        return loop;
    }

    void TServer::Wait() {
        HttpServer->Wait();
        if (IsAppHostEnabled) {
            Loop->SyncStopFork();
        }
        AtomicSet(IsRunning, 0);
        {
            auto itsWorkerThreadGuard = ITSWorkerThread.Access();
            auto& itsWorkerThread = *itsWorkerThreadGuard;
            if (itsWorkerThread) {
                itsWorkerThread->Join();
            }
            itsWorkerThread.Reset();
        }
    }

    void TServer::Stop() {
        OnStop();
        if (IsAppHostEnabled) {
            Loop->SyncStopFork();
        }
        HttpServer->Stop();
        AtomicSet(IsRunning, 0);
        {
            auto itsWorkerThreadGuard = ITSWorkerThread.Access();
            auto& itsWorkerThread = *itsWorkerThreadGuard;
            if (itsWorkerThread) {
                itsWorkerThread->Join();
            }
            itsWorkerThread.Reset();
        }
    }

    void TServer::Shutdown() {
        OnShutdown();
        if (IsAppHostEnabled) {
            Loop->SyncStopFork();
        }
        HttpServer->Shutdown();
        AtomicSet(IsRunning, 0);
        {
            auto itsWorkerThreadGuard = ITSWorkerThread.Access();
            auto& itsWorkerThread = *itsWorkerThreadGuard;
            if (itsWorkerThread) {
                itsWorkerThread->Join();
            }
            itsWorkerThread.Reset();
        }
    }

    void TServer::ReopenLogs() {
        if (Config.HasLogFile()) {
            GET_ERRORLOG.ReopenLog();
        }
        OnReopenLogs();
    }

    void TServer::OnWait() {}

    bool TServer::IsRun() const {
        return AtomicGet(IsRunning);
    }

    TServerStats& TServer::GetStats() {
        return ServerStats;
    }

    ui32 TServer::GetThreads() const {
        return Config.GetThreads();
    }

    ui32 TServer::GetApphostThreads() const {
        return AppHostThreads;
    }

    TClientRequest* TServer::CreateClient() {
        return new TRequest(*this);
    }

    void* TServer::CreateThreadSpecificResource() {
        return new TThreadResource();
    }

    void TServer::DestroyThreadSpecificResource(void* resource) {
        if (resource) {
            TThreadResource* res = reinterpret_cast<TThreadResource*>(resource);
            delete res;
        }
    }

    void TServer::OnFailRequest(int) {
        ServerStats.HitFailQueueReject();
    }

    void TServer::OnFailRequestEx(const TFailLogData&) {
        ServerStats.HitRequestQueueReject();
    }

    void TServer::OnException() {
        SEARCH_ERROR << "[TServer] http server exception " << CurrentExceptionMessage();
    }

    void TServer::InitHttpServerOptionsFromConfig() {
        HttpServerOptions.SetPort(Config.GetPort())
            .SetThreads(Config.GetThreads())
            .SetMaxQueueSize(Config.GetMaxQueueSize())
            .EnableCompression(Config.GetCompressionEnabled())
            .EnableKeepAlive(Config.GetKeepAliveEnabled())
            .EnableElasticQueues(Config.GetElasticQueueEnabled());

        if (Config.HasClientTimeout()) {
            HttpServerOptions.SetClientTimeout(
                TDuration::Parse(Config.GetClientTimeout())
            );
        }

        for (const auto& address : Config.GetBindAddress()) {
            TStringBuf host;
            TStringBuf portStr;
            ui16 port;
            if (TStringBuf(address).TryRSplit(":", host, portStr) && TryFromString(portStr, port)) {
                HttpServerOptions.AddBindAddress(TString(host), port);
            } else {
                HttpServerOptions.AddBindAddress(address);
            }
        }

        HttpServerOptions.MaxConnections = Config.GetMaxConnections();
        HttpServerOptions.MaxFQueueSize = Config.GetMaxFQueueSize();
        if (Config.HasListenBacklog()) {
            HttpServerOptions.SetListenBacklog(Config.GetListenBacklog());
        }
    }

    void TServer::InitStatsFromConfig() {
        const auto& intervalsConf = Config.GetResponseTimeIntervals();
        TVector<double> intervals;
        for (const auto& it : StringSplitter(intervalsConf).Split(',')) {
            intervals.push_back(FromString<double>(it.Token()));
        }
        ServerStats.Init(Config.GetStatsPrefix(), intervals);
    }

    THolder<IApphostRequest> TServer::CreateApphostClient() {
        Y_ASSERT(false);
        Y_ENSURE(false, "[TServer] CreateApphostClient is not implemented");
        return nullptr;
    }


    TMaybe<size_t> TServer::GetRequestQueueObjectCount() const {
        if (HttpServer) {
            if (const auto* x = dynamic_cast<const TElasticQueue*>(&HttpServer->GetRequestQueue())) {
                return x->ObjectCount();
            }
        }
        return {};
    }

    TMaybe<size_t> TServer::GetFailQueueObjectCount() const {
        if (HttpServer) {
            if (const auto* x = dynamic_cast<const TElasticQueue*>(&HttpServer->GetFailQueue())) {
                return x->ObjectCount();
            }
        }
        return {};
    }

} // namespace NServer
