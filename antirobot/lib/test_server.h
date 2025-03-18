#pragma once

#include <library/cpp/testing/unittest/tests_data.h>

#include "host_addr.h"

#include <library/cpp/http/server/http.h>

#include <util/generic/ptr.h>

namespace NAntiRobot {

struct TTestServer {
    THolder<THttpServer> Server;
    THostAddr Host;
    TPortManager PortManager;

    explicit TTestServer(THttpServer::ICallBack& callback) {
        ui16 port = PortManager.GetPort();
        Server.Reset(new THttpServer(&callback, THttpServerOptions(port)));
        if (Server->Start()) {
            TString addr = "localhost:" + ToString(port);
            Host = CreateHostAddr(addr);
            Host.HostName = std::move(addr);
            return;
        }
        Y_FAIL("Failed to find a free port to start the server");
    }

    TTestServer(TTestServer&& that) noexcept
        : Server(std::move(that.Server))
        , Host(std::move(that.Host))
    {}

    TTestServer& operator=(TTestServer&& that) noexcept {
        Server = std::move(that.Server);
        Host = std::move(that.Host);
        return *this;
    }

    ~TTestServer() {
        if (Server) {
            Server->Shutdown();
        }
    }
};


}
