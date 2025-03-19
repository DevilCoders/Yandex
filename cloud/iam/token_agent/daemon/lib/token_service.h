#pragma once

#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>

#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.grpc.pb.h>

#include "role_cache.h"
#include "token.h"

namespace NTokenAgent {
    namespace NProtoIam = yandex::cloud::priv::iam::v1;

    class TTokenServiceImpl final: public NProtoIam::TokenAgent::Service {
    public:
        explicit TTokenServiceImpl(const TRoleCache& roleCache)
                : RoleCache(roleCache)
        {
        }

        grpc::Status GetToken(grpc::ServerContext* context,
                              const NProtoIam::GetTokenRequest* request,
                              NProtoIam::GetTokenResponse* response) override;

    private:
        const TRoleCache& RoleCache;
    };
}
