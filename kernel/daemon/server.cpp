#include "server.h"

namespace NController {

    TExtendedServer::TExtendedServer(THolder<IServer>&& server, const IServerConfig& config)
        : Server(std::move(server))
        , Modules(config)
    {
    }

    void TExtendedServer::Run() {
        INFO_LOG << "Starting server..." << Endl;
        Server->Run();
        INFO_LOG << "Starting modules..." << Endl;
        Modules.Start();
        NOTICE_LOG << "Starting finished" << Endl;
    }

    void TExtendedServer::Stop(ui32 rigidStopLevel /*= 0*/) {
        INFO_LOG << "Stopping modules..." << Endl;
        IDaemonModule::TStopContext context;
        context.RigidLevel = rigidStopLevel;
        Modules.Stop(context);
        INFO_LOG << "Stopping server..." << Endl;
        Server->Stop(rigidStopLevel);
        INFO_LOG << "Stopping finished" << Endl;
    }
}
