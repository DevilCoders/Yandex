#include "../src/cloud-auth/client.h"
#include "../ut/mock-server.h"
#include "../src/internal.h"

#include <grpcpp/grpcpp.h>

using namespace yandex::cloud::auth;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
    gpr_log_verbosity_init();
    auto user_agent = std::string("as-client-test") + "/" + internal::VERSION;
    std::cout << user_agent
              << ", GRPC/" << grpc_version_string()
              << ", GRPC++/" << grpc::Version()
              << std::endl;

    const auto& endpoint = argc > 1 ? argv[1] : "inproc";
    const auto& permission = argc > 2 ? argv[2] : "resource-manager.clouds.get";
    const auto& resource_id = argc > 3 ? argv[3] : "eden";
    const auto& resource_type = argc > 4 ? argv[4] : "resource-manager.cloud";
    const auto& iam_token = std::getenv("YC_IAM_TOKEN");

    Credentials credentials(IamToken(iam_token ? iam_token : "MASTER-TOKEN"));

    AccessServiceClientConfig cfg{
        .ClientName = user_agent,
        .Endpoint = endpoint,
        .KeepAliveTime = 60s,
        .KeepAliveTimeout = 10s,
        .Plaintext = true,
        .RetryPolicy = {
            .MaxAttempts = 7,
            .InitialBackOff = 0.1s,
            .MaxBackOff = 2s,
            .BackoffMultiplier = 1.6,
            .RetryableStatusCodes = {
                "ABORTED",
                "CANCELLED",
                "DEADLINE_EXCEEDED",
                "INTERNAL",
                "UNAVAILABLE",
                "UNKNOWN",
            }},
        .RootCertificate = "....",
        .SslTargetNameOverride = "as.private-api.cloud.yandex.net"};

    std::shared_ptr<AccessServiceClient> client;
    std::shared_ptr<AccessServiceAsyncClient> asyncClient;

    std::shared_ptr<grpc::Server> server;
    if (std::strcmp("inproc", endpoint) == 0) {
        grpc::ChannelArguments channelArgs;
        channelArgs.SetUserAgentPrefix(std::string("as-mock") + "/" + internal::VERSION);

        server = CreateMockServer();
        std::shared_ptr<grpc::Channel> channel = server->InProcessChannel(channelArgs);

        client = internal::CreateAccessServiceClient(channel, cfg);
        asyncClient = internal::CreateAccessServiceAsyncClient(channel, cfg);
    } else {
        client = AccessServiceClient::Create(cfg);
        asyncClient = AccessServiceAsyncClient::Create(cfg);
    }

    auto result = client->Authenticate(credentials);
    std::cout << "Authenticate: " << result << std::endl;

    auto authorizeResult = client->Authorize(credentials, permission, Resource(resource_id, resource_type));
    std::cout << "Authorize: " << authorizeResult;

    if (authorizeResult) {
        std::cout << " succeeded!" << std::endl;
    } else {
        std::cout << " failed: " << authorizeResult.GetError() << std::endl;
    }

    auto asyncResult = asyncClient->Authorize(credentials, permission, Resource(resource_id, resource_type));
    std::cout << "Authorize (future): " << asyncResult.get() << std::endl;

    asyncClient->Authenticate(credentials, [](const auto& result) {
        std::cout << "Authenticate (callback): " << result << std::endl;
    });

    // Wait for **some** callbacks to finish.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    asyncClient.reset();
    return 0;
}
