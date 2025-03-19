#pragma once

#include <array>
#include <memory>
#include <asio.hpp>

#include "response.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"

namespace NHttpServer {
    class TUdsServer;

    class TConnection : public std::enable_shared_from_this<TConnection> {
    public:
        using TProtocol = asio::local::stream_protocol;
        using TSocket = TProtocol::socket;
        using TPtr = std::shared_ptr<TConnection>;

        TConnection(const TConnection&) = delete;
        TConnection &operator=(const TConnection&) = delete;

        explicit TConnection(TSocket socket, TUdsServer& server, const TRequestHandler& requestHandler);

        void Start();
        void Stop();

        const TSocket& GetSocket() const {
            return Socket;
        }

        const TRequest& GetRequest() const {
            return Request;
        }

    private:
        void DoRead();
        void DoWrite(const TResponse& Response);

        TSocket Socket;
        TUdsServer& Server;

        TRequestParser RequestParser;
        std::array<char, 8192> Buffer;
        TRequest Request;

        const TRequestHandler& RequestHandler;
    };
}
