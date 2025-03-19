#pragma once

#include "http_server/connection.hpp"
#include "http_server/server.hpp"
#include "role_cache.h"

namespace NTokenAgent {
    class THttpTokenService {
    public:
        explicit THttpTokenService(const TRoleCache& roleCache)
                : RoleCache(roleCache)
        {
        }

        void RegisterHandlers(NHttpServer::TRequestHandler& requestHandler) const;

    private:
        NHttpServer::TResponse GetIamToken(const NHttpServer::TConnection& connection) const;

    private:
        const TRoleCache& RoleCache;
    };
}
