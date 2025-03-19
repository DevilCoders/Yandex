#include "neh_server.h"

#include <library/cpp/logger/global/global.h>

#include <util/thread/pool.h>

namespace NUtil {

    class TServiceActor: public NNeh::IService {
    private:
        TAbstractNehServer* Server;

    public:
        TServiceActor(TAbstractNehServer* server)
            : Server(server)
        {
        }

        virtual void ServeRequest(const NNeh::IRequestRef& request) override {
            Y_ASSERT(Server);
            Server->ServeRequest(request);
        }
    };

    TAbstractNehServer::TAbstractNehServer(const TOptions& config)
        : Config(config)
        , Requester(NNeh::CreateLoop())
        , RequestCounter(0)
        , Started(false)
    {
        VERIFY_WITH_LOG(!config.Schemes.empty(), "incorrect config passed to neh server: no schemes");
        for (auto&& scheme : config.Schemes) {
            Requester->Add(scheme + "://*:" + ToString(config.Port) + "/*", MakeIntrusive<TServiceActor>(this));
        }
    }

    TAbstractNehServer::~TAbstractNehServer() {
        CHECK_WITH_LOG(!Started);
    }

    void TAbstractNehServer::ServeRequest(const NNeh::IRequestRef& req) {
        if (!Started) {
            return;
        }

        const ui64 id = AtomicIncrement(RequestCounter);

        THolder<IObjectInQueue> clientRequest(CreateClientRequest(id, req));
        Y_ASSERT(clientRequest);

        TFakeThreadPool fake;
        CHECK_WITH_LOG(fake.Add(clientRequest.Release()));
    }

    void TAbstractNehServer::Start() {
        DEBUG_LOG << "Start " << Config.GetServerName() << " server" << Endl;
        Requester->ForkLoop(Config.nThreads);
        Started = true;
        DEBUG_LOG << "Start " << Config.GetServerName() << " server... OK" << Endl;
    }

    void TAbstractNehServer::Stop() {
        DEBUG_LOG << "Stop " << Config.GetServerName() << " server" << Endl;
        Started = false;
        Requester->SyncStopFork();
        DEBUG_LOG << "Stop " << Config.GetServerName() << " server... OK" << Endl;
    }

}
