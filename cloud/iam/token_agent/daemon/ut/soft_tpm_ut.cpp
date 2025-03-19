#include <library/cpp/testing/gtest/gtest.h>
#include "soft_tpm.h"
#include <yaml-cpp/yaml.h>
#include "mon.h"
#include "config.h"
#include <grpcpp/grpcpp.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include "ssl_util.h"
#include <filesystem>
#include <fstream>


#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1/tpm_agent_service.grpc.pb.h>

namespace NProtoTpmAgent = yandex::cloud::priv::infra::tpmagent::v1;

const ui16 MON_PORT = 0;
const std::string OK_PASSWORD = "PASSWORD";
//password: length must be in the range of 1 to 32
const std::string TOO_LONG_PASSWORD = "PASSWORDPASSWORDPASSWORDPASSWORDPASSWORD";
const long HANDLE = 111111111111;

enum EFunctionToTest {
    FTT_CREATE,
    FTT_READ_PUBLIC,
    FTT_SIGN
};
struct TTestSoftTpmData {
    EFunctionToTest function;
    std::string password;
    NProtoTpmAgent::Hierarchy hierarchy;
    NProtoTpmAgent::Alg algorithm;
    NProtoTpmAgent::Hash hash;
    grpc::StatusCode expectedStatusCode;
};

class TSoftTpmFixture: public testing::TestWithParam<TTestSoftTpmData> {
protected:
    void SetUp() {
        NTokenAgent::TMon::Start(MON_PORT);
        auto config = NTokenAgent::TConfig(
                "",
                YAML::Load("keyPath: " + GetTempFolderName().string())
                );
        softTpmAgentService = std::make_unique<NTokenAgent::TSoftTpmAgentImpl>(config);
        instance = grpc::ServerBuilder()
                .RegisterService(softTpmAgentService.get())
                .BuildAndStart();
        auto TpmAgentChannel = instance->InProcessChannel(grpc::ChannelArguments{});
        TpmAgent = NProtoTpmAgent::TpmAgent::NewStub(TpmAgentChannel);
    }
    void TearDown() {
        instance->Shutdown();
        NTokenAgent::TMon::Stop();
        std::filesystem::remove_all(random_folder_name);
    }

    std::filesystem::path GetTempFolderName() {
        if (random_folder_name.empty()) {
            auto path = std::filesystem::temp_directory_path()
                        / "soft_tpm_test";
            std::filesystem::create_directories(path);
            random_folder_name.swap(path);
        }
        return random_folder_name;
    }

    std::unique_ptr<NTokenAgent::TSoftTpmAgentImpl> softTpmAgentService;
    std::unique_ptr<grpc::Server> instance;
    std::shared_ptr<NProtoTpmAgent::TpmAgent::Stub> TpmAgent;
    std::filesystem::path random_folder_name;
};

TEST_P(TSoftTpmFixture, Test) {
    auto params = GetParam();

    NProtoTpmAgent::CreateRequest createRequest;
    NProtoTpmAgent::CreateResponse createReply;
    grpc::ClientContext createContext;
    createRequest.set_password(params.password.c_str(), params.password.size());
    createRequest.Sethierarchy(params.hierarchy);
    auto createStatus = TpmAgent->Create(&createContext, createRequest, &createReply);
    switch (params.function) {
        case FTT_CREATE: {
            EXPECT_EQ(createStatus.error_code(), params.expectedStatusCode);
            if (params.expectedStatusCode == grpc::StatusCode::OK) {
                EXPECT_FALSE(createReply.pub().empty());
            }
        }
            break;
        case FTT_READ_PUBLIC: {
            NProtoTpmAgent::ReadPublicRequest request;
            NProtoTpmAgent::ReadPublicResponse reply;
            grpc::ClientContext context;
            if (params.expectedStatusCode == grpc::StatusCode::OK) {
                request.set_handle(createReply.handle());
            } else if (params.expectedStatusCode == grpc::StatusCode::INTERNAL) {
                std::ofstream(GetTempFolderName() / (std::to_string(HANDLE) + ".pub")) << "";
                request.set_handle(HANDLE);
            }
            auto status = TpmAgent->ReadPublic(&context, request, &reply);
            EXPECT_EQ(status.error_code(), params.expectedStatusCode);
            if (params.expectedStatusCode == grpc::StatusCode::OK) {
                EXPECT_FALSE(reply.pub().empty());
            }

        }
            break;
        case FTT_SIGN: {
            NProtoTpmAgent::SignRequest request;
            NProtoTpmAgent::SignResponse reply;
            grpc::ClientContext context;

            request.set_password(params.password.c_str(), params.password.size());
            request.mutable_scheme()->set_alg(params.algorithm);
            request.mutable_scheme()->set_hash(params.hash);

            auto hash = NTokenAgent::Sha256Hash("data");
            request.set_digest(hash.c_str(), hash.size());

            if (params.expectedStatusCode == grpc::StatusCode::OK) {
                request.set_handle(createReply.handle());
            } else if (params.expectedStatusCode == grpc::StatusCode::INTERNAL) {
                std::ofstream(GetTempFolderName() / std::to_string(HANDLE)) << "";
                request.set_handle(HANDLE);
            }

            auto status = TpmAgent->Sign(&context, request, &reply);
            EXPECT_EQ(status.error_code(), params.expectedStatusCode);
            if (params.expectedStatusCode == grpc::StatusCode::OK) {
                EXPECT_FALSE(reply.signature().empty());
            }
        }
            break;
    }

}

INSTANTIATE_TEST_SUITE_P(
    SoftTpmTest,
    TSoftTpmFixture,
    ::testing::Values(
            TTestSoftTpmData{
                FTT_CREATE,
                OK_PASSWORD,
                NProtoTpmAgent::Hierarchy::OWNER,
                NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                grpc::StatusCode::OK,
            }, TTestSoftTpmData{
                    FTT_CREATE,
                    "",
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::INVALID_ARGUMENT,
            } ,TTestSoftTpmData{
                    FTT_CREATE,
                    TOO_LONG_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::INVALID_ARGUMENT,
            }, TTestSoftTpmData{
                    FTT_CREATE,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::HIERARCHY_UNSPECIFIED,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::INVALID_ARGUMENT,
            }, TTestSoftTpmData{
                    FTT_READ_PUBLIC,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::NOT_FOUND,
            }, TTestSoftTpmData{
                    FTT_READ_PUBLIC,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::OK,
            }, TTestSoftTpmData{
                    FTT_READ_PUBLIC,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::INTERNAL,
            }, TTestSoftTpmData{
                    FTT_SIGN,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::RSASSA,
                    NProtoTpmAgent::Hash::SHA256,
                    grpc::StatusCode::OK,
            },TTestSoftTpmData{
                    FTT_SIGN,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::RSAPSS,
                    NProtoTpmAgent::Hash::SHA256,
                    grpc::StatusCode::OK,
            },TTestSoftTpmData{
                    FTT_SIGN,
                    TOO_LONG_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::INVALID_ARGUMENT,
            }, TTestSoftTpmData{
                    FTT_SIGN,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::ALG_UNSPECIFIED,
                    NProtoTpmAgent::Hash::SHA256,
                    grpc::StatusCode::OUT_OF_RANGE,
            }, TTestSoftTpmData{
                    FTT_SIGN,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::RSASSA,
                    NProtoTpmAgent::Hash::HASH_UNSPECIFIED,
                    grpc::StatusCode::OUT_OF_RANGE,
            }, TTestSoftTpmData{
                    FTT_SIGN,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::RSASSA,
                    NProtoTpmAgent::Hash::SHA256,
                    grpc::StatusCode::NOT_FOUND,
            }, TTestSoftTpmData{
                    FTT_SIGN,
                    OK_PASSWORD,
                    NProtoTpmAgent::Hierarchy::OWNER,
                    NProtoTpmAgent::Alg::RSASSA,
                    NProtoTpmAgent::Hash::SHA256,
                    grpc::StatusCode::INTERNAL,
            }));
