#include <server.h>
#include <library/cpp/testing/gtest/gtest.h>
#include <library/cpp/logger/global/global.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <pwd.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.grpc.pb.h>

const std::string GOOD_TAG = "GOOD_TAG";
const std::string UNKNOWN_TAG = "UNKNOWN";
const std::string NO_TAG_TOKEN = "NO_TAG_TOKEN";
const std::string TAG_TOKEN = "TAG_TOKEN";


struct TServerTestData {
    std::string tag;
    std::string log_file;
};

class TServerFixture: public ::testing::TestWithParam<TServerTestData> {
protected:
    void SetUp() {
        auto config = NTokenAgent::TConfig::FromFile(CreateConfig());
        server = std::make_unique<NTokenAgent::TServer>(config);
        server_thread = std::make_unique<std::thread>([this] {
            server->RunServer();
        });
        server->UpdateTokens();
    }
    void TearDown() {
        if (server) {
            server->StopServer();
        }
        if (server_thread) {
            server_thread->join();
        }
        server.reset();
        std::filesystem::remove_all(GetTempFolderName());
    }
    std::filesystem::path CreateConfig() {
        auto temp_path = GetTempFolderName();
        auto config_path = temp_path / "config.yaml";
        std::ofstream(config_path)
                << "cacheUpdateInterval: 1s\n"
                   "keyPath: " << temp_path / "key" << "\n"
                   "cachePath: " << temp_path / "cache" << "\n"
                   "rolesPath: " << temp_path / "roles.d" << "\n"
                   "groupRolesPath: " << temp_path / "groups.d" << "\n"
                   "listenUnixSocket:\n"
                   "  path: " << temp_path / "grpc.socket" << "\n"
                   "monitoringPort: 0\n"
                   "tokenServiceEndpoint:\n"
                   "  host: localhost\n"
                   "  port: 0\n"
                   "  useTls: false\n"
                   "  timeout: 1s";
        auto current_user = std::string(::getpwuid(::getuid())->pw_name);
        auto roles_path = temp_path / "roles.d";
        auto current_user_role_path = roles_path / current_user;
        std::filesystem::create_directories(current_user_role_path);
        std::ofstream(roles_path / (current_user + ".yaml")) << ("token: " + NO_TAG_TOKEN + "\n");
        std::ofstream(current_user_role_path / (GOOD_TAG + ".yaml")) << ("token: " + TAG_TOKEN + "\n");

        std::filesystem::create_directories(temp_path / "groups.d");

        return config_path;
    }

    std::filesystem::path GetTempFolderName() {
        if (random_folder_name.empty()) {
            auto path = std::filesystem::temp_directory_path()
                        / "server_test";
            std::filesystem::create_directories(path);
            std::filesystem::permissions(path, std::filesystem::perms::owner_all);
            random_folder_name.swap(path);
        }
        return random_folder_name;
    }

    std::filesystem::path GetGrpcSocketPath() {
        return GetTempFolderName() / "grpc.socket";
    }
private:
    std::unique_ptr<NTokenAgent::TServer> server;
    std::unique_ptr<std::thread> server_thread;
    std::filesystem::path random_folder_name;
};



TEST_P(TServerFixture, TagTest) {
    auto params = GetParam();
    auto tag = params.tag;
    auto log_file = params.log_file;

    if (!log_file.empty()) {
        log_file = (GetTempFolderName() / log_file).string();
    }

    DoInitGlobalLog(NTokenAgent::LogBackend(log_file, TLOG_DEBUG));

    grpc::ClientContext context;
    yandex::cloud::priv::iam::v1::GetTokenRequest request;
    yandex::cloud::priv::iam::v1::GetTokenResponse reply;
    grpc::ChannelArguments args;
    auto credentials = grpc::InsecureChannelCredentials();
    auto channel = grpc::CreateCustomChannel("unix://" + GetGrpcSocketPath().string(), credentials, args);

    auto stub = yandex::cloud::priv::iam::v1::TokenAgent::NewStub(channel);
    if (!tag.empty()) {
        request.mutable_tag()->assign(tag);
    }
    grpc::Status status = stub->GetToken(&context, request, &reply);

    if (tag == "") {
        EXPECT_EQ(0, (int) status.error_code());
        EXPECT_EQ(NO_TAG_TOKEN, reply.iam_token());
    } else if (tag == GOOD_TAG) {
        EXPECT_EQ(0, (int) status.error_code());
        EXPECT_EQ(TAG_TOKEN, reply.iam_token());
    } else {
        EXPECT_EQ(7, (int) status.error_code());
    }
}

INSTANTIATE_TEST_SUITE_P(
        ServerTest,
        TServerFixture,
        ::testing::Values(
                TServerTestData{
                    "",
                    ""
                },
                TServerTestData{
                    GOOD_TAG,
                    ""
                },
                TServerTestData{
                    UNKNOWN_TAG,
                    ""
                },
                TServerTestData{
                        "",
                        "log_file"
                }));

TEST(ServerSocketTest, FailBindSocketTest) {
    auto config = NTokenAgent::TConfig("", YAML::Load("listenUnixSocket:\n  path: \n"));
    EXPECT_THROW_MESSAGE_HAS_SUBSTR( {
        try {
            auto server = NTokenAgent::TServer(config);
        } catch (const yexception& e) {
            throw;
        }
    }, yexception, "Failed to bind socket" );
}
