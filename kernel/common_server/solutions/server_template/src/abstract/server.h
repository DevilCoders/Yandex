#pragma once
#include <fintech/risk/common/src/abstract/server.h>

namespace NCS {
    namespace NServerTemplate {
        class IServer: public virtual IBaseServer {
        public:
        };
    }
}

using IServerTemplate = NCS::NServerTemplate::IServer;
