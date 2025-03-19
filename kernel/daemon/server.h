#pragma once

#include "module/module.h"

class TBomb;

namespace NController {

    class IServer {
    public:
        virtual ~IServer() {
        }
        virtual void Run() = 0;
        virtual void Stop(ui32 rigidStopLevel, const TCgiParameters* cgiParams = nullptr) = 0;

        template<class T>
        inline T& GetMeAs() {
            T* result = dynamic_cast<T*>(this);
            CHECK_WITH_LOG(result);
            return *result;
        }

        template<class T>
        inline const T& GetMeAs() const {
            const T* result = dynamic_cast<const T*>(this);
            CHECK_WITH_LOG(result);
            return *result;
        }
    };

    class TExtendedServer {
    private:
        THolder<IServer> Server;
        TDaemonModules Modules;

    public:
        TExtendedServer(THolder<IServer>&& server, const IServerConfig& config);

        IServer& GetLogicServer() {
            return *Server;
        }

        void Run();
        void Stop(ui32 rigidStopLevel = 0);
    };
}
