#pragma once

#include <asio.hpp>
#include <set>
#include <string>

#include <util/generic/yexception.h>

#include "../config.h"
#include "connection.hpp"
#include "request_handler.hpp"

namespace NHttpServer {
    class TUdsServer {
    public:
        using TProtocol = asio::local::stream_protocol;
        using TSocket = TProtocol::socket;
        using TAcceptor = TProtocol::acceptor;
        using TReqHandler = std::function<TResponse(const TConnection&)>;

        TUdsServer(const TUdsServer&) = delete;
        TUdsServer& operator=(const TUdsServer&) = delete;

        explicit TUdsServer();

        template <typename T>
        TUdsServer& AddService(const T* service) {
            if (ServerThread.joinable()) {
                ythrow TSystemError() << "HTTP Server already started";
            }

            service->RegisterHandlers(RequestHandler);

            return *this;
        }

        void Start(const NTokenAgent::TListenUnixSocketConfig& config);
        void Stop();

    protected:
        friend class TConnection;
        void StopConnection(const TConnection::TPtr& connection);

    private:
        void Initialize(const NTokenAgent::TListenUnixSocketConfig& config);
        void Run();

        void DoAccept();

        void StartConnection(const TConnection::TPtr& connection);
        void StopAllConnections();

        std::thread ServerThread;
        asio::io_context IoContext;
        TAcceptor Acceptor;

        std::set<TConnection::TPtr> Connections;
        TRequestHandler RequestHandler;
    };
}
