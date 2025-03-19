#include "mock_iam_token_service.h"
#include <grpcpp/grpcpp.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/iam_token_service.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/iam_token_service.grpc.pb.h>
#include <contrib/libs/jwt-cpp/include/jwt-cpp/jwt.h>

class MockIamTokenService: public yandex::cloud::priv::iam::v1::IamTokenService::Service {
public:
    MockIamTokenService(const TMockIamTokenServiceConfig& cfg)
        : cfg(cfg)
    {
    }

    ::grpc::Status Create(::grpc::ServerContext*,
                          const ::yandex::cloud::priv::iam::v1::CreateIamTokenRequest* request,
                          ::yandex::cloud::priv::iam::v1::CreateIamTokenResponse* response) override {
        const auto serviceAccountId = jwt::decode(request->jwt()).get_issuer();
        if (serviceAccountId == cfg.UnauthenticatedServiceAccountId) {
            return grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "Test unauthenticated error"};
        } else if (serviceAccountId == cfg.UnknownServiceAccountId) {
            return grpc::Status{grpc::StatusCode::UNKNOWN, "Test unknown error"};
        } else {
            response->set_iam_token(cfg.IamToken.c_str());
            auto timestamp = new google::protobuf::Timestamp();
            timestamp->set_seconds(cfg.ExpiresAt);
            timestamp->set_nanos(0);
            response->set_allocated_expires_at(timestamp);
            return grpc::Status{};
        }
    }

    TMockIamTokenServiceConfig cfg;
};

std::shared_ptr<grpc::Server> CreateMockIamTokenServer(const TMockIamTokenServiceConfig& cfg) {
    return grpc::ServerBuilder().RegisterService(new MockIamTokenService(cfg)).BuildAndStart();
}
