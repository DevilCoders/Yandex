#include <utility>

#include "server.hpp"
#include "connection.hpp"
#include "request_handler.hpp"
#include "ucred_option.hpp"

namespace NHttpServer {
    TConnection::TConnection(TSocket socket, TUdsServer& server, const TRequestHandler& requestHandler)
        : Socket(std::move(socket))
        , Server(server)
        , RequestHandler(requestHandler)
    {
    }

    void TConnection::Start() {
        DoRead();
    }

    void TConnection::Stop() {
        Socket.close();
    }

    void TConnection::DoRead() {
        auto self(shared_from_this());
        Socket.async_read_some(asio::buffer(Buffer),[this, self](std::error_code ec, std::size_t bytesTransferred) {
            if (!ec) {
                TRequestParser::EParseResult result =
                    RequestParser.parse(Request, Buffer.data(), Buffer.data() + bytesTransferred);

                switch (result) {
                    case TRequestParser::EParseResult::Good:
                        DoWrite(RequestHandler.HandleRequest(*this));
                        break;

                    case TRequestParser::EParseResult::Bad:
                        DoWrite(TResponse::GetStockReply(TResponse::EStatus::BadRequest));
                        break;

                    default:
                        DoRead();
                }
            } else if (ec != asio::error::operation_aborted) {
                Server.StopConnection(shared_from_this());
            }
        });
    }

    void TConnection::DoWrite(const TResponse& response) {
        auto self(shared_from_this());
        asio::async_write(Socket, response.ToBuffers(),[this, self](std::error_code ec, std::size_t) {
            if (!ec) {
                asio::error_code ignored;
                Socket.shutdown(TSocket::shutdown_both,ignored);
            }

            if (ec != asio::error::operation_aborted) {
                Server.StopConnection(shared_from_this());
            }
        });
    }
}
