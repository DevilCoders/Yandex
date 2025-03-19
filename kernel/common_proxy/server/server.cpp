#include "server.h"
#include <kernel/common_proxy/unistat_signals/signals.h>
#include <kernel/daemon/bomb.h>
#include <kernel/daemon/messages.h>

namespace NCommonProxy {

    TServer::TServer(const TConfig& config)
        : Config(config)
    {
        RegisterGlobalMessageProcessor(this);
        try {
            for (const auto& proc : Config.GetProccessorsConfigs().GetModules()) {
                try {
                    Processors[proc] = TProcessor::Create(proc, Config.GetProccessorsConfigs());
                } catch (...) {
                    ythrow yexception() << "Error while create processor " << proc << ": " << CurrentExceptionMessage();
                }
            }
            for (const auto& link : Config.GetLinks()) {
                try {
                    if (!link.Enabled) {
                        continue;
                    }
                    TProcessor::TPtr from = Processors[link.From];
                    TProcessor::TPtr to = Processors[link.To];
                    TLink::TPtr l = MakeIntrusive<TLink>(*from, *to, link);
                    from->AddListener(l);
                    to->AddRequester(l);
                } catch (...) {
                    ythrow yexception() << "Error while create link from " << link.From << " to " << link.To << ": " << CurrentExceptionMessage();
                }
            }

            TCommonProxySignals::TProcessors initializers;
            for (auto& proc : Processors) {
                initializers.push_back(proc.second.Get());
            }
            TCommonProxySignals::BuildSignals(initializers, Config.GetUnistatSignals());

            for (auto& proc : Processors) {
                try {
                    proc.second->Init();
                } catch (...) {
                    ythrow yexception() << "Error while init processor " << proc.first << ": " << CurrentExceptionMessage();
                }
            }
        } catch (...) {
            TString msg = CurrentExceptionMessage();
            FAIL_LOG("%s", msg.data());
        }
    }

    TServer::~TServer() {
        UnregisterGlobalMessageProcessor(this);
    }

    const TServer::TConfig& TServer::GetConfig() const {
        return Config;
    }

    void TServer::Run() {
        TBomb bomb(Config.GetStartTimeout(), "He knew too much...");
        for (auto& proc : Processors) {
            try {
                proc.second->Start();
            } catch (...) {
                ythrow yexception() << "Error while start processor " << proc.first << ": " << CurrentExceptionMessage();
            }
        }
        for (auto& proc : Processors) {
            try {
                proc.second->Run();
            } catch (...) {
                ythrow yexception() << "Error while run processor " << proc.first << ": " << CurrentExceptionMessage();
            }
        }
        bomb.Deactivate();
        AtomicSet(Working, 1);
    }

    void TServer::Stop(ui32 /*rigidStopLevel*/, const TCgiParameters* /*cgiParams*/) {
        AtomicSet(Working, 0);
        for (auto& proc : Processors) {
            try {
                proc.second->Stop();
            } catch (...) {
                ythrow yexception() << "Error while stop processor " << proc.first << ": " << CurrentExceptionMessage();
            }
        }
        for (auto& proc : Processors) {
            try {
                proc.second->Wait();
            } catch (...) {
                ythrow yexception() << "Error while wait processor " << proc.first << ": " << CurrentExceptionMessage();
            }
        }
    }

    bool TServer::Process(IMessage* message) {
        if (dynamic_cast<TMessageUpdateUnistatSignals*>(message)) {
            TCommonProxySignals::PushAliveSignal(AtomicGet(Working));
            return true;
        }
        return false;
    }

    TString TServer::Name() const {
        return "CommonProxyServer";
    }

}
