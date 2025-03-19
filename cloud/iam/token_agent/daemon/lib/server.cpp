#include <grpcpp/server_posix.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/logger/filter.h>
#include <library/cpp/logger/rotating_file.h>
#include <util/generic/size_literals.h>
#include <util/generic/yexception.h>

#include "config.h"
#include "http_server/server.hpp"
#include "logging_interceptor.h"
#include "mon.h"
#include "server.h"
#include "soft_tpm.h"
#include "token_service.h"
#include "updater.h"

namespace NTokenAgent {
    static std::unique_ptr<grpc::Server> _instance;
    static std::atomic_bool _running;

    static TStreamSocket OpenSocket(const TListenUnixSocketConfig& config) {
        auto& path = config.GetPath();
        auto mode = config.GetMode();
        auto group = config.GetGroup();
        auto end_point = TSockAddrLocal(path.c_str());
        TLocalStreamSocket socket;

        if (socket.Bind(&end_point, mode)) {
            ythrow TSystemError() << "Failed to bind socket " << path;
        }

        if (group != gid_t(-1)) {
            if (::chown(path.c_str(), -1, group)) {
                ythrow TSystemError() << "Failed to set socket group" << path;
            }
        }

        if (socket.Listen(config.GetBackLog())) {
            ythrow TSystemError() << "Failed to listen on socket " << path;
        }

        INFO_LOG << "Server listening on " << path
                 << " socket " << socket.operator SOCKET() << "\n";
        return std::move(socket);
    }

    TServer::TServer(const TConfig& config)
            : socket(OpenSocket(config.GetListenUnixSocket()))
    {
        TMon::Start(config.GetMonitoringPort());

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;

        tokenService = std::make_unique<TTokenServiceImpl>(roleCache);

        // Use builtin TPM emulator if no TPM agent configured
        if (config.GetTpmAgentEndpoint().IsInproc()) {
            softTpmAgentService = std::make_unique<TSoftTpmAgentImpl>(config);
            INFO_LOG << "With builtin TPM emulator\n";
            builder.RegisterService(softTpmAgentService.get());
        }

        std::vector<std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>> creators;
        creators.push_back(std::make_unique<NTokenAgent::LoggingInterceptorFactory>());
        builder.experimental().SetInterceptorCreators(std::move(creators));

        _instance = builder
                .RegisterService(tokenService.get())
                .BuildAndStart();

        _running = true;

        iamTokenClient = std::make_shared<TIamTokenClient>(config);
        updater = std::make_unique<TTokenUpdater>(config, iamTokenClient, roleCache);

        if (!config.GetHttpListenUnixSocket().GetPath().empty()) {
            httpServer = std::make_unique<NHttpServer::TUdsServer>();
            httpTokenService = std::make_unique<THttpTokenService>(roleCache);
            httpServer
                    ->AddService(httpTokenService.get())
                    .Start(config.GetHttpListenUnixSocket());
            INFO_LOG << "HTTP server started at " << config.GetHttpListenUnixSocket().GetPath() << "\n";
        } else {
            INFO_LOG << "HTTP server disabled\n";
        }

    }

    TServer::~TServer() {
        _running = false;
        if (httpServer) {
            httpServer->Stop();
        }
        TMon::Stop();
        _instance->Shutdown();
        _instance->Wait();
    }

    void TServer::RunServer() {
        struct sockaddr_un addr {};
        socklen_t len = sizeof(addr);

        while (!socket.Closed()) {
            auto fd = ::accept4(socket, (sockaddr*)&addr, &len, SOCK_NONBLOCK);
            if (fd > 0) {
                grpc::AddInsecureChannelFromFd(_instance.get(), fd);
            }
        }
    }

    void TServer::StopServer() {
        _running = false;
        INFO_LOG << "Shutting down the server\n";
        socket.ShutDown(SHUT_RD);
        socket.Close();
    }

    void TServer::UpdateTokens() {
        updater->ScheduleUpdateImmediately();
    }

    bool TServer::IsRunning() {
        return _running;
    }

    ui32 GetUserId(const grpc::ServerContext* context) {
        auto peer = context->peer();
        DEBUG_LOG << "Peer name " << peer << "\n";
        // peer name should be in 'fd:###' format
        if (peer.find("fd:") != 0) {
            ythrow yexception() << "Expected 'fd:' prefix";
        }
        // CLOUD-18224: rewrite this after upgrade to GRPC 1.14
        auto fd = std::stoi(peer.substr(3));

        ucred peercred{};
        socklen_t peercred_len = sizeof(peercred);
        Y_ENSURE(::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peercred, &peercred_len) == 0);

        DEBUG_LOG << "User id " << peercred.uid
                  << " group id " << peercred.gid
                  << " process id " << peercred.pid
                  << "\n";
        return peercred.uid;
    }

    std::shared_ptr<grpc::Channel> InProcessChannel(const grpc::ChannelArguments& args) {
        while (!_instance) {
            INFO_LOG << "Waiting for the server instance";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return _instance->InProcessChannel(args);
    }

    THolder<TLogBackend> LogBackend(const std::string& path, ELogPriority priority) {
        THolder<TLogBackend> backend;
        if (path.empty()) {
            backend = MakeHolder<TStreamLogBackend>(&Cerr);
        } else {
            backend = MakeHolder<TRotatingFileLogBackend>(path.c_str(), 10_MB, 10);
        }
        if (priority != LOG_MAX_PRIORITY) {
            backend = MakeHolder<TFilteredLogBackend>(std::move(backend), priority);
        }
        return backend;
    }
}
