#pragma once
#include <fintech/risk/common/src/abstract/server.h>

namespace NCS {
    namespace NFallbackProxy {
        class IServer: public virtual IBaseServer {
        public:
        };
    }
}

using IFallbackProxy = NCS::NFallbackProxy::IServer;
