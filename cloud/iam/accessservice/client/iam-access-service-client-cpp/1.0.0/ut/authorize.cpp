#include "mock-server.h"
#include "../src/cloud-auth/client.h"
#include "../src/internal.h"

#include <grpcpp/grpcpp.h>
#include <library/cpp/testing/gtest/gtest.h>

using namespace yandex::cloud::auth;

class AuthorizeFixture: public ::testing::TestWithParam<std::pair<std::string, AuthorizationResult>> {
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

TEST_P(AuthorizeFixture, Authorize) {
    auto client = internal::CreateAccessServiceClient(server->InProcessChannel(grpc::ChannelArguments{}), cfg);
    auto params = GetParam();
    auto result = client->Authorize(IamToken(params.first), "permission", Resource::Folder("folder"));
    EXPECT_EQ(result, params.second);
}

TEST_P(AuthorizeFixture, AsyncAuthorize) {
    auto client = internal::CreateAccessServiceAsyncClient(server->InProcessChannel(grpc::ChannelArguments{}), cfg);
    auto params = GetParam();
    auto future = client->Authorize(IamToken(params.first), "permission", Resource::Folder("folder"));
    auto result = future.get();
    EXPECT_EQ(result, params.second);
}

INSTANTIATE_TEST_SUITE_P(
    ClientTests,
    AuthorizeFixture,
    ::testing::Values(
        std::make_pair("OK:ANONYMOUS", AuthorizationResult{AuthStatus::OK, Anonymous{}}),
        std::make_pair("OK:UA:UA_ID:FEDERATION_ID", AuthorizationResult{AuthStatus::OK, UserAccount{"UA_ID", "FEDERATION_ID"}}),
        std::make_pair("OK:SA:SA_ID:FOLDER_ID", AuthorizationResult{AuthStatus::OK, ServiceAccount{"SA_ID", "FOLDER_ID"}}),
        std::make_pair("UNKNOWN:Message:Details", AuthorizationResult{Error{grpc::StatusCode::UNKNOWN, "Message", "Details"}, Anonymous{}}),
        std::make_pair("UNAUTHENTICATED:ANONYMOUS", AuthorizationResult{Error{grpc::StatusCode::UNAUTHENTICATED, "", ""}, Anonymous{}}),
        std::make_pair("PERMISSION_DENIED:SA:SA_ID:FOLDER_ID", AuthorizationResult{Error{grpc::StatusCode::PERMISSION_DENIED, "", ""}, ServiceAccount{"SA_ID", "FOLDER_ID"}}),
        std::make_pair("CANCELLED:Message:Details", AuthorizationResult{Error{grpc::StatusCode::CANCELLED, "Message", "Details"}, Anonymous{}})));
