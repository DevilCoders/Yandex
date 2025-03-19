#include <library/cpp/testing/gtest/gtest.h>
#include "config.h"
#include "updater.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include "mock/mock_iam_token_service.h"
#include "mock/mock_tpm_agent_service.h"
#include "mon.h"
#include "server.h"
#include <library/cpp/logger/global/global.h>
#include <yaml-cpp/yaml.h>

const ui16 MON_PORT = 0;
const std::string TOKEN = "TOKEN";
const std::string SERVICE_ACCOUNT_ID = "service_account";
const unsigned long EXPIRES_AT = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch())
                                         .count() +
                                 60 * 60 * 24 * 7;

//use default password from tpm_sign.cpp
const TMockTpmAgentConfig MOCK_TPM_AGENT_CONFIG = {
        "token-agent",
        "INCORRECT_PASSWORD",
        "SIGNATURE"
};

const TMockIamTokenServiceConfig MOCK_IAM_TOKEN_SERVICE_CONFIG = {
        "",
        "",
        "IAM_TOKEN",
        EXPIRES_AT
};

struct TTestRoles {
    std::string Name;
    std::string Token;
    std::string ExpiresAt;
    std::string KeyId;
};

const TTestRoles ROLE_TO_ADD = TTestRoles{
        "full_role",
        TOKEN,
        "",
        "keyId"
};

const auto CACHED_ROLE_NAME = "cached_role";

std::vector<TTestRoles> roles = {
        TTestRoles{
            "empty",
            "",
            "",
            ""
            }
        , TTestRoles{
            "no_token",
            "",
            "",
            "keyId"
            }
        , TTestRoles{
            "no_key_id",
            TOKEN,
            "",
            ""
            }
        , TTestRoles{
            "old_token",
            TOKEN,
            "1970-01-01T00:00:00.000000Z",
            ""
        },TTestRoles{
                CACHED_ROLE_NAME,
                "",
                "",
                "keyId"
        }};

class TUpdaterFixture: public ::testing::Test {
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

    std::filesystem::path CreateConfig() {
        auto temp_path = GetTempFolderName();
        auto config_path = temp_path / "config.yaml";
        std::ofstream(config_path)
                << "cacheUpdateInterval: 1s\n"
                   "cachePath: " << temp_path / "cache" << "\n"
                   "monitoringPort: 0\n";

        roles_path = temp_path / "roles.d";
        std::filesystem::create_directories(roles_path);
        for (auto& role : roles) {
            std::ofstream(roles_path / (role.Name + ".yaml")) << ("token: " + role.Token
                + "\nexpiresAt: " + role.ExpiresAt
                + "\nkeyId: " + role.KeyId
                + "\nserviceAccountId: " + SERVICE_ACCOUNT_ID + "\n");
        }

        auto cached_role_path = temp_path / "cache";
        std::filesystem::create_directories(cached_role_path);
        std::ofstream(cached_role_path / CACHED_ROLE_NAME) << TOKEN << "\n" << "2023-01-01T00:00:00.000000Z";

        std::filesystem::create_directories(temp_path / "groups.d");

        INFO_LOG << cached_role_path << "\n";
        return config_path;
    }

    std::filesystem::path GetTempFolderName() {
        if (random_folder_name.empty()) {
            auto path = std::filesystem::temp_directory_path()
                        / "updater_test";
            std::filesystem::create_directories(path);
            random_folder_name.swap(path);
        }
        return random_folder_name;
    }

    std::filesystem::path random_folder_name;
    std::filesystem::path roles_path;
    NTokenAgent::TServer* server;
};

TEST_F(TUpdaterFixture, Test) {
    auto config = NTokenAgent::TConfig::FromFile(CreateConfig());
    NTokenAgent::TRoleCache roleCache;

    auto TpmAgentServer = CreateMockTpmAgentServer(MOCK_TPM_AGENT_CONFIG);
    auto TpmAgentChannel = TpmAgentServer->InProcessChannel(grpc::ChannelArguments{});

    auto IamTokenServer = CreateMockIamTokenServer(MOCK_IAM_TOKEN_SERVICE_CONFIG);
    auto IamTokenServiceChannel = IamTokenServer->InProcessChannel(grpc::ChannelArguments{});
    auto iamTokenClient = std::make_shared<NTokenAgent::TIamTokenClient>(config, IamTokenServiceChannel, TpmAgentChannel);

    auto updater = NTokenAgent::TTokenUpdater(config, iamTokenClient, roleCache);
    updater.ScheduleUpdateImmediately();

    while (roleCache.GetRoleIds().size() != roles.size() - 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::ofstream(roles_path / (ROLE_TO_ADD.Name + ".yaml")) << ("token: " + ROLE_TO_ADD.Token
           + "\nexpiresAt: " + ROLE_TO_ADD.ExpiresAt
           + "\nkeyId: " + ROLE_TO_ADD.KeyId
           + "\nserviceAccountId: " + SERVICE_ACCOUNT_ID + "\n");

    auto tokenRoleToAdd = roleCache.FindToken( NTokenAgent::USER_PREFIX + ROLE_TO_ADD.Name);
    while (tokenRoleToAdd.IsEmpty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        tokenRoleToAdd = roleCache.FindToken( NTokenAgent::USER_PREFIX + ROLE_TO_ADD.Name);
    }

    std::filesystem::remove(roles_path / (ROLE_TO_ADD.Name + ".yaml"));

    while (roleCache.GetRoleIds().size() != roles.size() - 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    for (auto& role : roles) {
        auto token = roleCache.FindToken(NTokenAgent::USER_PREFIX + role.Name);
        if (role.Token.empty() && role.KeyId.empty()) {
            EXPECT_TRUE(token.IsEmpty());
        } else {
            while (token.IsEmpty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                token = roleCache.FindToken(NTokenAgent::USER_PREFIX + role.Name);
            }
            if (!role.Token.empty()) {
                EXPECT_EQ(token.GetValue(), role.Token);
            } else {
                EXPECT_EQ(token.GetValue(), MOCK_IAM_TOKEN_SERVICE_CONFIG.IamToken);
            }
        }
    }
    server->StopServer();
}
