#pragma once
#include <grpcpp/server.h>

extern std::shared_ptr<grpc::Server> CreateMockServer();
