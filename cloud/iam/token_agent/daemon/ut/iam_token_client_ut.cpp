#include "mock/mock_iam_token_service.h"
#include "mock/mock_tpm_agent_service.h"
#include <library/cpp/testing/gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include <library/cpp/logger/global/global.h>
#include <filesystem>
#include "iam_token_client.h"
#include "config.h"
#include "role.h"
#include "iam_token_client_utils.h"
#include "mon.h"
#include "server.h"

//DEFAULT_PASSWORD from tpm_sign.cpp
const std::string DEFAULT_PASSWORD("token-agent");
const int IAM_TOKEN_SERVICE_RETRIES = 5;
const int TPM_AGENT_RETRIES = 5;
const std::string IAM_TOKEN = "iam_token";
const std::string SERVICE_ACCOUNT_ID = "service_account";
const std::string SIGNATURE = "signature";
const std::string USER_NAME = "username";
const ui16 MON_PORT = 0;
const unsigned long EXPIRES_AT = std::chrono::duration_cast<std::chrono::seconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count() +
                                 60 * 60 * 24 * 7;

struct TTestData {
    TMockIamTokenServiceConfig IamTokenServiceCfg;
    TMockTpmAgentConfig TpmAgentCfg;

    std::variant<std::pair<std::string, unsigned long>, std::string> Expect;
};

class TIamTokenClientFixture: public ::testing::TestWithParam<TTestData> {
protected:
    void SetUp() {
        InitGlobalLog2Console(TLOG_DEBUG);
        NTokenAgent::TMon::Start(MON_PORT);
        server = new NTokenAgent::TServer(NTokenAgent::TConfig("", YAML::Load(
                "listenUnixSocket:\n"
                "  path: " + (GetTempFolderName() / "server.socket").string() + "\n"
                "monitoringPort: 0\n"
                "keyPath: " + (GetTempFolderName() / "key").string() + "\n"
                "cachePath: " + (GetTempFolderName() / "cache").string() + "\n")));
    }
    void TearDown() {
        NTokenAgent::TMon::Stop();
        std::filesystem::remove_all(GetTempFolderName());
    }

    std::filesystem::path GetTempFolderName() {
        if (random_folder_name.empty()) {
            auto path = std::filesystem::temp_directory_path()
                        / "iam_token_client_test";
            std::filesystem::create_directories(path);
            random_folder_name.swap(path);
        }
        return random_folder_name;
    }

    std::filesystem::path random_folder_name;
    NTokenAgent::TServer* server;
};

TEST_P(TIamTokenClientFixture, CreateIamToken) {
    auto params = GetParam();

    auto TpmAgentServer = CreateMockTpmAgentServer(params.TpmAgentCfg);
    auto TpmAgentChannel = TpmAgentServer->InProcessChannel(grpc::ChannelArguments{});

    auto IamTokenServer = CreateMockIamTokenServer(params.IamTokenServiceCfg);
    auto IamTokenServiceChannel = IamTokenServer->InProcessChannel(grpc::ChannelArguments{});

    auto config = NTokenAgent::TConfig("",
                                       YAML::Load(
                                           "tokenServiceEndpoint:\n retries: " + std::to_string(IAM_TOKEN_SERVICE_RETRIES) + "\ntpmAgentEndpoint:\n retries: " + std::to_string(TPM_AGENT_RETRIES) + "\n"));
    auto role = NTokenAgent::TRole(USER_NAME,
                                   YAML::Load(
                                       "token: " + IAM_TOKEN + "\n" + "serviceAccountId: " + SERVICE_ACCOUNT_ID + "\n"));

    auto IamTokenClient = NTokenAgent::TIamTokenClient(config, IamTokenServiceChannel, TpmAgentChannel);

    auto* expectedValues = std::get_if<0>(&params.Expect);
    if (expectedValues != nullptr) {
        auto response = IamTokenClient.CreateIamToken(role);
        EXPECT_EQ(response.GetValue(), expectedValues->first);
        if (expectedValues->second != 0) {
            EXPECT_EQ(response.GetExpiresAt().Seconds(), expectedValues->second);
        }
    } else {
        auto throwMessage = std::get<1>(params.Expect);
        EXPECT_THROW_MESSAGE_HAS_SUBSTR({
            try {
                auto response = IamTokenClient.CreateIamToken(role);
            } catch (...) {
                throw;
            }
        }, yexception, throwMessage);
    }
    //NTokenAgent::TServer::Running(false);
}

TMockIamTokenServiceConfig CreateMockIamTokenServiceConfig(const std::string unauthenticatedSA, const std::string unknownSA) {
    return TMockIamTokenServiceConfig{
        unauthenticatedSA,
        unknownSA,
        IAM_TOKEN,
        EXPIRES_AT};
}

TMockTpmAgentConfig CreateMockTpmAgentConfig(bool goodConfig) {
    auto cfg = TMockTpmAgentConfig{
        DEFAULT_PASSWORD,
        "",
        SIGNATURE};
    std::string password = NTokenAgent::TIamTokenClientUtils::ReadPassword();
    if (password.empty()) {
        password = DEFAULT_PASSWORD;
    }
    //using password to make sure tests won't accidentally fail
    if (goodConfig) {
        cfg.IncorrectPassword = password + "incorrect_password";
    } else {
        cfg.DefaultPassword = password + "default_password";
        cfg.IncorrectPassword = password;
    }
    return cfg;
}

INSTANTIATE_TEST_SUITE_P(
    IamTokenServiceTest,
    TIamTokenClientFixture,
    ::testing::Values(
        TTestData{
            CreateMockIamTokenServiceConfig("", ""),
            CreateMockTpmAgentConfig(true),
            std::make_pair(IAM_TOKEN, EXPIRES_AT)},
        TTestData{
            CreateMockIamTokenServiceConfig(SERVICE_ACCOUNT_ID, ""),
            CreateMockTpmAgentConfig(true),
            std::make_pair("", 0)},
        TTestData{
            CreateMockIamTokenServiceConfig("", SERVICE_ACCOUNT_ID),
            CreateMockTpmAgentConfig(true),
            "Failed to create IAM token"},
        TTestData{
            CreateMockIamTokenServiceConfig("", ""),
            CreateMockTpmAgentConfig(false),
            "TPM failed"}));
