#pragma once

#include "cloud-auth/client.h"

#include <grpcpp/channel.h>

namespace yandex::cloud::auth::internal {

    constexpr auto USER_AGENT = "as-client";
    constexpr auto VERSION = "1.0";

    extern std::string GetExecutableName();

    extern std::string GetExecutableModificationDate();

    extern std::shared_ptr<grpc::Channel> CreateChannel(
        const AccessServiceClientConfig& config);

    extern std::shared_ptr<AccessServiceAsyncClient> CreateAccessServiceAsyncClient(
        std::shared_ptr<grpc::Channel> channel,
        const AccessServiceClientConfig& config);

    extern std::shared_ptr<AccessServiceClient> CreateAccessServiceClient(
        std::shared_ptr<grpc::Channel> channel,
        const AccessServiceClientConfig& config);

}
