#pragma once
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>

#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/iam_token_service.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/iam_token_service.grpc.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.grpc.pb.h>

#include "role.h"
#include "token.h"

class TFsPath;
namespace NTokenAgent {
    namespace NProtoIam = yandex::cloud::priv::iam::v1;
    namespace NProtoTpmAgent = yandex::cloud::priv::infra::tpmagent::v1;
    class TConfig;
    class TRole;
    class TIamTokenClient {
    public:
        explicit TIamTokenClient(const TConfig& config, const std::shared_ptr<grpc::Channel>& IamTokenServiceChannel,
                                 const std::shared_ptr<grpc::Channel>& TpmAgentChannel);
        explicit TIamTokenClient(const TConfig& config);
        TToken CreateIamToken(const TRole& role);

    private:
        TToken CreateIamToken(const std::string& jwt);
        std::string CreateJwt(const TRole& role);

    private:
        TDuration JwtLifetime;
        TDuration TsRequestTimeout;
        int TsRetries;
        TDuration TpmRequestTimeout;
        int TpmRetries;
        std::string JwtAudience;
        std::unique_ptr<NProtoIam::IamTokenService::Stub> IamTokenService;
        std::shared_ptr<NProtoTpmAgent::TpmAgent::Stub> TpmAgent;
    };
}
