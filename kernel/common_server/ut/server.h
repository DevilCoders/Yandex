#pragma once

#include "const.h"

#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/network/neh.h>

#include <library/cpp/mediator/messenger.h>
#include <library/cpp/resource/resource.h>
#include <util/generic/cast.h>
#include <util/system/env.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NServerTest {

    class IServerGuard {
    public:
        virtual ~IServerGuard() {
        }

        virtual const IBaseServer* GetServer() const = 0;

        template <class T>
        const T& GetAs() const {
            return *VerifyDynamicCast<const T*>(GetServer());
        }
    };

    template <class TServer, class TServerConfig>
    class TServerGuard: public IServerGuard {
    protected:
        TAtomicSharedPtr<TServer> Server;

    public:
        using TConfig = TServerConfig;

        virtual const IBaseServer* GetServer() const override {
            return Server.Get();
        }

        const TServer& operator*() const {
            return *Server;
        }

        TServer& operator*() {
            return *Server;
        }

        TServer* operator->() {
            return Server.Get();
        }

        const TServer* operator->() const {
            return Server.Get();
        }

        TServer* Get() {
            return Server.Get();
        }

        TServerGuard(TServerConfig& config)
            : Server(MakeAtomicShared<TServer>(config))
        {
            TDBEventLogGuard::SetActive(false);
            Server->Run();
        }

        virtual ~TServerGuard() {
            Server->Stop(0);
        }
    };

    class TSimpleClient {
        NSimpleMeta::TConfig MetaConfig;
        THolder<NNeh::THttpClient> NehAgent;

    public:
        TSimpleClient(ui16 serverPort) {
            MetaConfig.SetMaxAttempts(1).SetGlobalTimeout(TDuration::Seconds(60));
            NehAgent = MakeHolder<NNeh::THttpClient>(MetaConfig);
            NehAgent->RegisterSource("test_server", "127.0.0.1", serverPort, MetaConfig);
        }

        template <class TRequest, class... TArgs>
        typename TRequest::TResponse SendRequest(TArgs... args) const {
            TRequest r(args...);
            return SendRequest(r, Now() + TDuration::Seconds(500));
        }

        template <class TRequest>
        typename TRequest::TResponse SendRequest(const TRequest& r, const TInstant deadline = Now() + TDuration::Seconds(5)) const {
            NNeh::THttpRequest request;
            r.BuildHttpRequest(request);
            NUtil::THttpReply report = NehAgent->SendMessageSync(request, deadline);
            typename TRequest::TResponse result;
            result.ParseReply(report);
            return result;
        }

        NJson::TJsonValue AskServer(const NNeh::THttpRequest& request, const TInstant deadline, bool parseBadReplies = false) const {
            NUtil::THttpReply result = NehAgent->SendMessageSync(request, deadline);

            NJson::TJsonValue resultReport = NJson::JSON_MAP;
            if ((result.Code() != 200 && !parseBadReplies) || !NJson::ReadJsonFastTree(result.Content(), &resultReport)) {
                ERROR_LOG << result.GetDebugReply() << ": " << result.Code() << ": " << result.Content() << Endl;
                return NJson::JSON_NULL;
            }
            return resultReport;
        }
    };
}
