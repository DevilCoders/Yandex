#pragma once

#include "apphost_request.h"
#include "config.h"
#include "itsworker.h"
#include "serverstat.h"

#include <apphost/api/service/cpp/service.h>

#include <library/cpp/http/server/options.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/http_ex.h>
#include <library/cpp/http/server/response.h>

#include <library/cpp/threading/synchronized/synchronized.h>

#include <kernel/searchlog/searchlog.h>
#include <kernel/server/protos/serverconf.pb.h>

#include <util/datetime/base.h>
#include <util/generic/fwd.h>
#include <util/stream/str.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/types.h>

namespace NServer {

    struct TThreadResource {
        TThreadResource()
            : HttpConfig(SharedHttpServerConfig.AtomicLoad())
            , ItsConfigBlob(SharedItsConfigBlob.AtomicLoad())
        {
        }

        TSharedHttpServerConfig::TPtr HttpConfig;
        TSharedItsConfigBlob::TPtr ItsConfigBlob;
    };

    class TServer;

    class TRequest: public THttpClientRequestEx {
    public:
        TRequest(TServer& parentWebdaemon);
        ~TRequest() override = default;

        // Special method for serverless tests
        void Reply(const TString& query, void* res);

        virtual bool NeedRegisterProcessingTime(const bool productionTraffic, const HttpCodes httpCode) const;
        virtual bool NeedRegisterProcessingStatus(const bool productionTraffic, const HttpCodes httpCode) const;

    protected:
        virtual bool ProcessHeaders();
        virtual bool DoReply(const TString& scriptName, THttpResponse& response);
        virtual THttpResponse TextResponse(TStringBuf content, HttpCodes httpCode = HTTP_OK) const;
        virtual const TString& CacheControlHeaderString() const;
        virtual void AssignResource(void* res);
        virtual bool HandleAdmin(const TString& action, THttpResponse& response);
        virtual void LoadItsConfig(const TBlob& configBlob);

        const TString& GetMethod() const {
            return Method;
        }

        const TServerRequestData& RequestData() const;

        bool IsHeadRequest() const {
            return HeadRequest;
        }

        const TInstant& GetModifiedSince() const {
            return ModifiedSince;
        }

        const TString& GetRequestDate() const {
            return RequestDate;
        }

        const TString& GetKeyFromETag() const {
            return KeyFromETag;
        }

    private:
        // TClientRequest interface implementation
        bool Reply(void* res) final;
        THttpResponse HandleScript();

        void BaseProcessMethod();
        bool BaseProcessHeaders();

        void BaseAssignResource(void* res);
        THttpResponse BaseHandleAdmin();
        THttpResponse HandleRemoteAdmin();
        THttpResponse HandleRobotsTxt();
        virtual THttpResponse HandleHealth() const;
        THttpResponse HandleMstat() const;
        THttpResponse HandleSvnRevision() const;
        THttpResponse AdminShutdown() const;
        THttpResponse AdminReopenLogs() const;

        bool IsTimeOutRequest() const;

    private:
        TString Method;
        bool HeadRequest = false;
        TInstant ModifiedSince;
        TString KeyFromETag;
        TString RequestDate;
        TInstant CreateTime = TInstant::Now();
        TThreadResource* ServerResource = nullptr;
        TServer& ParentWebdaemon;
    };

    class TServer: public THttpServer::ICallBack {
    public:
        explicit TServer(const TString& configPath);
        explicit TServer(const NServer::THttpServerConfig& config);
        ~TServer() override = default;

        void Start();
        void Wait();
        void Stop();
        void Shutdown();
        void ReopenLogs();
        bool IsRun() const;
        TServerStats& GetStats();

        const NServer::THttpServerConfig& GetConfig() const {
            return Config;
        }
        ui32 GetThreads() const;
        ui32 GetApphostThreads() const;

        size_t GetRequestQueueSize() const {
            return HttpServer ? HttpServer->GetRequestQueueSize() : 0;
        }

        size_t GetApphostQueueSize() const {
            return Loop ? Loop->GetStats().EnqueuedRequests : 0;
        }

        size_t GetFailQueueSize() const {
            return HttpServer ? HttpServer->GetFailQueueSize() : 0;
        }

        TMaybe<size_t> GetRequestQueueObjectCount() const;
        TMaybe<size_t> GetFailQueueObjectCount() const;

        // AppHost support
        void EnableAppHost(ui32 port, ui32 threads);

        // ICallBack interface implementation
        TClientRequest* CreateClient() override;
        void* CreateThreadSpecificResource() override;
        void DestroyThreadSpecificResource(void* resource) override;
        void OnFailRequest(int failstate) override;
        void OnFailRequestEx(const TFailLogData& d) override;
        void OnException() override;
        void OnWait() override;

    private:
        void DoInit();
        void InitHttpServerOptionsFromConfig();
        void InitStatsFromConfig();

        virtual THolder<IApphostRequest> CreateApphostClient();
        void ReplyApphost(NAppHost::IServiceContext& ctx);

        void* GetThreadLocalResource();

        void EnableAppHost(ui32 port, ui32 threads, bool grpcEnabled, ui32 grpcPort, ui32 grpcThreadCount);

        // To be overriden in childs
        virtual void OnStart() {}
        virtual void OnStop() {}
        virtual void OnShutdown() {}
        virtual void OnReopenLogs() {}
        virtual THolder<NAppHost::TLoop> CreateApphostLoop();

    private:
        NServer::THttpServerConfig Config;
        TAtomic IsRunning;
        THttpServer::TOptions HttpServerOptions;
        THolder<THttpServer> HttpServer;
        TServerStats ServerStats;
        NThreading::TSynchronized<THolder<IThreadFactory::IThread>, TMutex> ITSWorkerThread;

        // AppHost support
        bool IsAppHostEnabled = false;
        ui32 AppHostPort = 0;
        ui32 AppHostThreads = 0;
        THolder<NAppHost::TLoop> Loop;

        // Apphost grpc support
        bool IsGrpcEnabled = false;
        ui32 GrpcPort = 0;
        size_t GrpcThreadCount = 0;
    };

} // namespace NServer
