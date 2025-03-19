#pragma once
#include <grpcpp/server.h>

struct TMockTpmAgentConfig {
    std::string DefaultPassword;
    std::string IncorrectPassword;
    std::string Signature;
};

extern std::shared_ptr<grpc::Server> CreateMockTpmAgentServer(const TMockTpmAgentConfig& cfg);
