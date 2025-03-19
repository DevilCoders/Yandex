#pragma once
#include <grpcpp/server.h>

struct TMockIamTokenServiceConfig {
    std::string UnauthenticatedServiceAccountId;
    std::string UnknownServiceAccountId;
    std::string IamToken;
    unsigned long ExpiresAt{};
};

extern std::shared_ptr<grpc::Server> CreateMockIamTokenServer(const TMockIamTokenServiceConfig& cfg);
