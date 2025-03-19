#pragma once

#include <string>
#include <library/cpp/logger/backend.h>
#include <util/generic/ptr.h>
#include <grpcpp/grpcpp.h>

#include <util/network/sock.h>
#include "http_token_service.h"
#include "iam_token_client.h"

namespace NTokenAgent {

    std::shared_ptr<grpc::Channel> InProcessChannel(const grpc::ChannelArguments& args);

    THolder<TLogBackend> LogBackend(const std::string& path, ELogPriority priority = LOG_MAX_PRIORITY);

    class TSoftTpmAgentImpl;
    class TTokenServiceImpl;
    class TTokenUpdater;

    class TServer {
    public:
        explicit TServer(const TConfig& config);
        ~TServer();
        void RunServer();
        void StopServer();
        void UpdateTokens();

        static bool IsRunning();
    private:
        std::unique_ptr<NHttpServer::TUdsServer> httpServer;
        std::unique_ptr<TSoftTpmAgentImpl> softTpmAgentService;
        std::unique_ptr<TTokenServiceImpl> tokenService;
        std::unique_ptr<THttpTokenService> httpTokenService;
        std::unique_ptr<TTokenUpdater> updater;
        std::shared_ptr<TIamTokenClient> iamTokenClient;
        TStreamSocket socket;
        TRoleCache roleCache;
        TRoleCache groupRoleCache;
    };
}
