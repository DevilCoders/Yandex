#pragma once

#include <filesystem>

#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>

#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.grpc.pb.h>

namespace NTokenAgent {
    namespace NTpmAgent = yandex::cloud::priv::infra::tpmagent::v1;
    class TConfig;
    class TSoftTpmAgentImpl final: public NTpmAgent::TpmAgent::Service {
        grpc::Status Create(grpc::ServerContext* context,
                            const NTpmAgent::CreateRequest* request,
                            NTpmAgent::CreateResponse* reply) override;

        grpc::Status ReadPublic(grpc::ServerContext* context,
                                const NTpmAgent::ReadPublicRequest* request,
                                NTpmAgent::ReadPublicResponse* reply) override;

        grpc::Status Sign(grpc::ServerContext* context,
                          const NTpmAgent::SignRequest* request,
                          NTpmAgent::SignResponse* reply) override;

    public:
        explicit TSoftTpmAgentImpl(const TConfig& config);

    private:
        std::filesystem::path KeyPath;
    };
}
