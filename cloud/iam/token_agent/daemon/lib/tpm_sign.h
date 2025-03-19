#pragma once

#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.grpc.pb.h>

namespace NTokenAgent {
    namespace NProtoTpmAgent = yandex::cloud::priv::infra::tpmagent::v1;

    class TTpmSign {
    public:
        TTpmSign(std::shared_ptr<NProtoTpmAgent::TpmAgent::Stub> tpm_agent,
                 ui64 key_handle,
                 std::string key_password,
                 int retries,
                 TDuration request_timeout);

        static std::string name() {
            return "RS256";
        }
        std::string sign(const std::string& data) const;

    private:
        std::shared_ptr<NProtoTpmAgent::TpmAgent::Stub> TpmAgent;
        ui64 KeyHandle;
        std::string KeyPassword;
        int Retries;
        TDuration RequestTimeout;
    };
}
