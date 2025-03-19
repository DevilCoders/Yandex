#include <iostream>

#include <library/cpp/logger/global/global.h>
#include <utility>
#include <util/generic/yexception.h>
#include <util/system/types.h>

#include "server.hpp"
#include "ucred_option.hpp"

namespace NHttpServer {
    TUdsServer::TUdsServer()
        : IoContext(1)
        , Acceptor(IoContext)
        , RequestHandler()
    {
    }

    void TUdsServer::Start(const NTokenAgent::TListenUnixSocketConfig& config) {
        Initialize(config);

        INFO_LOG << "Starting HTTP Server thread\n";
        ServerThread = std::thread(&TUdsServer::Run, this);
    }

    void TUdsServer::Stop() {
        if (ServerThread.joinable()) {
            INFO_LOG << "Stopping HTTP Server thread\n";

            asio::post(IoContext, [this](){
                Acceptor.close();
                StopAllConnections();
            });

            ServerThread.join();
            INFO_LOG << "HTTP Server thread stopped\n";
        }
    }

    void TUdsServer::Initialize(const NTokenAgent::TListenUnixSocketConfig& config) {
        INFO_LOG << "Initialize HTTP Server thread\n";

        auto& socketPath = config.GetPath();

        ::unlink(socketPath.c_str());
        Acceptor.open();
        Acceptor.set_option(TSocket::reuse_address(true));
        Acceptor.bind(socketPath);

        if (config.GetGroup() != gid_t(-1)) {
            if (::chown(socketPath.c_str(), -1, config.GetGroup())) {
                ythrow TSystemError() << "Failed to set socket group " << socketPath;
            }
        }

        if (::chmod(socketPath.c_str(), config.GetMode())) {
            ythrow TSystemError() << "Failed to set socket mode " << socketPath;
        }

        Acceptor.listen(config.GetBackLog());

        INFO_LOG << "HTTP Server listening on " << socketPath << " socket " << Acceptor.native_handle() << "\n";
    }

    void TUdsServer::Run() {
        INFO_LOG << "HTTP Server thread started\n";
        DoAccept();
        IoContext.run();
    }

    void TUdsServer::DoAccept() {
        Acceptor.async_accept([this](std::error_code ec, TSocket socket) {
            if (Acceptor.is_open()) {
                DoAccept();

                if (!ec) {
                    TUcredOption ucredOption{};
                    socket.get_option(ucredOption);
                    DEBUG_LOG << "Accept connection from user id " << ucredOption.Uid()
                              << " group id " << ucredOption.Gid()
                              << " process id " << ucredOption.Pid()
                              << "\n";

                    StartConnection(std::make_shared<TConnection>(std::move(socket), *this, RequestHandler));
                }
            }
        });
    }

    void TUdsServer::StartConnection(const TConnection::TPtr& connection) {
        Connections.insert(connection);
        connection->Start();
    }

    void TUdsServer::StopConnection(const TConnection::TPtr& connection) {
        Connections.erase(connection);
        connection->Stop();
    }

    void TUdsServer::StopAllConnections() {
        for (const auto& connection : Connections) {
            connection->Stop();
        }

        Connections.clear();
    }
}
