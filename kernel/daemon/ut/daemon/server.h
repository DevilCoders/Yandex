#pragma once
#include "config.h"
#include <kernel/daemon/base_controller.h>
#include <kernel/daemon/messages.h>

class TCollectSearchProxyServerInfo: public TCollectServerInfo {
};

class TController: public NController::TController {
private:
    using TBase = NController::TController;
public:
    using TBase::TBase;
};

class TServer: public NController::IServer {
private:
    const TConfig& Config;
public:
    using TController = ::TController;
    using TConfig = ::TConfig;
    using TInfoCollector = ::TCollectSearchProxyServerInfo;
    TServer(const TConfig& config)
        : Config(config)
    {

    }
    virtual void Run() {
        AssertCorrectIndex(Config.GetState() != "FAILED_INDEX", "Incorrect index");
        CHECK_WITH_LOG(Config.GetState() != "FAILED_SERVER");
    }

    virtual void Stop(ui32 /*rigidStopLevel*/, const TCgiParameters* /*cgiParams*/ = nullptr) {
    }
};
