#include "mock_tpm_agent_service.h"
#include <grpcpp/grpcpp.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.grpc.pb.h>

class MockTpmAgentService: public yandex::cloud::priv::infra::tpmagent::v1::TpmAgent::Service {
public:
    MockTpmAgentService(const TMockTpmAgentConfig& cfg)
            : cfg(cfg)
    {
    }

    ::grpc::Status Sign(::grpc::ServerContext* ,
                        const ::yandex::cloud::priv::infra::tpmagent::v1::SignRequest* request,
                        ::yandex::cloud::priv::infra::tpmagent::v1::SignResponse* response) override {
        auto password = request->password();
        if (password == cfg.DefaultPassword) {
            response->set_signature(cfg.Signature.c_str());
            return grpc::Status{};
        } else if (password == cfg.IncorrectPassword) {
            return grpc::Status{grpc::StatusCode::UNKNOWN, "Test unknown error"};
        } else {
            return grpc::Status{grpc::StatusCode::PERMISSION_DENIED, "Test permission denied error"};
        }
    }
    TMockTpmAgentConfig cfg;
};

std::shared_ptr<grpc::Server> CreateMockTpmAgentServer(const TMockTpmAgentConfig& cfg) {
    return grpc::ServerBuilder().RegisterService(new MockTpmAgentService(cfg)).BuildAndStart();
}
