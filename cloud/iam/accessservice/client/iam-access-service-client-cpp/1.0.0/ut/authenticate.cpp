#include "mock-server.h"
#include "../src/cloud-auth/client.h"
#include "../src/internal.h"

#include <grpcpp/grpcpp.h>
#include <library/cpp/testing/gtest/gtest.h>

using namespace yandex::cloud::auth;

class AuthenticateFixture: public ::testing::TestWithParam<std::pair<std::string, AuthenticationResult>> {
protected:
    std::shared_ptr<grpc::Server> server = CreateMockServer();
    AccessServiceClientConfig cfg{
        .ClientName = std::string("as-client-ut") + "/" + internal::VERSION,
        .RetryPolicy{
            .MaxAttempts = 3,
            .InitialBackOff = std::chrono::duration<double>::zero(),
            .MaxBackOff = std::chrono::seconds{1},
            .BackoffMultiplier = 1.0,
            .RetryableStatusCodes = {}}};
};

TEST_P(AuthenticateFixture, Authenticate) {
    auto client = internal::CreateAccessServiceClient(server->InProcessChannel(grpc::ChannelArguments{}), cfg);
    auto params = GetParam();
    auto result = client->Authenticate(IamToken(params.first));
    EXPECT_EQ(result, params.second);
}

TEST_P(AuthenticateFixture, AsyncAuthenticate) {
    auto client = internal::CreateAccessServiceAsyncClient(server->InProcessChannel(grpc::ChannelArguments{}), cfg);
    auto params = GetParam();
    auto future = client->Authenticate(IamToken(params.first));
    auto result = future.get();
    EXPECT_EQ(result, params.second);
}

INSTANTIATE_TEST_SUITE_P(
    AuthenticateTests,
    AuthenticateFixture,
    ::testing::Values(
        std::make_pair("OK:ANONYMOUS", AuthenticationResult{AuthStatus::OK, Anonymous{}}),
        std::make_pair("OK:UA:UA_ID:FEDERATION_ID", AuthenticationResult{AuthStatus::OK, UserAccount{"UA_ID", "FEDERATION_ID"}}),
        std::make_pair("OK:SA:SA_ID:FOLDER_ID", AuthenticationResult{AuthStatus::OK, ServiceAccount{"SA_ID", "FOLDER_ID"}}),
        std::make_pair("UNAUTHENTICATED:ANONYMOUS", AuthenticationResult{Error{grpc::StatusCode::UNAUTHENTICATED, "", ""}, Anonymous{}}),
        std::make_pair("UNAVAILABLE:Message:Details", AuthenticationResult{Error{grpc::StatusCode::UNAVAILABLE, "Message", "Details"}, Anonymous{}})));
