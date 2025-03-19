#pragma once

#include "config.h"
#include <kernel/common_proxy/common/processor.h>
#include <kernel/daemon/base_controller.h>

namespace NCommonProxy {

    class TServer : public NController::IServer, public IMessageProcessor {
    public:
        using TController = NController::TController;
        using TConfig = TServerConfig;

    public:
        typedef TCollectServerInfo TInfoCollector;
        TServer(const TConfig& config);
        ~TServer();
        const TConfig& GetConfig() const;
        void Run() override;
        void Stop(ui32 /*rigidStopLevel*/, const TCgiParameters* /*cgiParams*/) override;
        //IMessageProcessor
        virtual bool Process(IMessage* message) override;
        virtual TString Name() const override;
    private:
        TAtomic Working = 0;
        const TConfig& Config;
        THashMap<TString, TProcessor::TPtr> Processors;
    };

}
