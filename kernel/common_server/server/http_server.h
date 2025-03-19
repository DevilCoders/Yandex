#pragma once

#include <kernel/common_server/library/searchserver/simple/server.h>
#include <kernel/common_server/server/client.h>

class IBaseServer;

class TFrontendHttpServer: public TCommonSearchServer<TFrontendClientRequest, TBaseServerConfig> {
private:
    using TBase = TCommonSearchServer<TFrontendClientRequest, TBaseServerConfig>;
    const TBaseServerConfig* Config;
    const IBaseServer* Server;

public:
    TFrontendHttpServer(const TBaseServerConfig& config, const IBaseServer* server)
        : TBase(config)
        , Config(&config)
        , Server(server)
    {
    }

    virtual TClientRequest* CreateClient() override {
        return new TFrontendClientRequest(Config, Server);
    }
};
