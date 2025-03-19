#pragma once

#include <apphost/api/service/cpp/service.h>

namespace NServer {
    class IApphostRequest {
    public:
        virtual ~IApphostRequest() = default;

    public:
        // virtual void Reply(NAppHost::IServiceContext& ctx, void* res) const = 0;
        virtual void Reply(NAppHost::IServiceContext& ctx, void* res) const = 0;
    };

} // namespace NServer
