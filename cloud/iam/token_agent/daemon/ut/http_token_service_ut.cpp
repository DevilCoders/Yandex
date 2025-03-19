#include <server.h>
#include <library/cpp/testing/gtest/gtest.h>
#include <library/cpp/logger/global/global.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <pwd.h>
#include <unistd.h>
#include <contrib/libs/curl/include/curl/curl.h>
#include "mon.h"
#include <library/cpp/json/json_reader.h>

const std::string TOKEN = "TOKEN";
const std::string URI_PATH = "tokenAgent/v1/token";
const std::string CURRENT_USER = std::string(::getpwuid(::getuid())->pw_name);
const ui16 MON_PORT = 0;

struct THttpTestData {
    std::string user;
};

class THttpServerFixture: public ::testing::TestWithParam<THttpTestData> {
protected:
    void SetUp() {
        InitGlobalLog2Console(TLOG_DEBUG);
        NTokenAgent::TMon::Start(MON_PORT);
    }

    void TearDown() {
        NTokenAgent::TMon::Stop();
        std::filesystem::remove_all(GetTempFolderName());
    }
    std::filesystem::path CreateConfig(const std::string user) {
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
                   "httpListenUnixSocket:\n"
                   "  path: " << temp_path / "http.socket" << "\n"
                   "monitoringPort: 0\n"
                   "tokenServiceEndpoint:\n"
                   "  host: localhost\n"
                   "  port: 0\n"
                   "  useTls: false\n"
                   "  timeout: 1s";
        auto roles_path = temp_path / "roles.d";
        auto user_role_path = roles_path / user;
        std::filesystem::create_directories(user_role_path);
        std::ofstream(roles_path / (user + ".yaml")) << ("token: " + TOKEN + "\n");

        return config_path;
    }

    std::filesystem::path GetTempFolderName() {
        if (random_folder_name.empty()) {
            auto path = std::filesystem::temp_directory_path()
                        / "http_server_test";
            std::filesystem::create_directories(path);
            random_folder_name.swap(path);
        }
        return random_folder_name;
    }

    std::filesystem::path GetHttpSocketPath() {
        return GetTempFolderName() / "http.socket";
    }

private:
    std::filesystem::path random_folder_name;
};

size_t CallBackWriteFunction(void *data, size_t size, size_t nmemb, std::string *s)
{
    size_t length = size * nmemb;
    try {
        s->append((char *) data, length);
    } catch(std::bad_alloc &e)
    {
        return 0;
    }
    return length;
}

TEST_P(THttpServerFixture, Test) {
    auto user = GetParam().user;
    auto config = NTokenAgent::TConfig::FromFile(CreateConfig(user));
    auto server = std::make_unique<NTokenAgent::TServer>(config);
    auto server_thread = std::make_unique<std::thread>([&server] {
        server->RunServer();
    });

    CURL *curlHandler;
    CURLcode responseCode;
    std::string response;

    curlHandler = curl_easy_init();

    if(curlHandler) {
        curl_easy_setopt(curlHandler, CURLOPT_UNIX_SOCKET_PATH, GetHttpSocketPath().c_str());
        curl_easy_setopt(curlHandler, CURLOPT_URL, ("http://localhost/" + URI_PATH).c_str());
        curl_easy_setopt(curlHandler, CURLOPT_WRITEFUNCTION, CallBackWriteFunction);
        curl_easy_setopt(curlHandler, CURLOPT_WRITEDATA, &response);

        responseCode = curl_easy_perform(curlHandler);

        NJson::TJsonValue jsonResponse;
        if (user == CURRENT_USER) {
            EXPECT_TRUE(NJson::ReadJsonTree(response, &jsonResponse));
            EXPECT_EQ(jsonResponse["iam_token"], TOKEN);
        } else {
            EXPECT_FALSE(NJson::ReadJsonTree(response, &jsonResponse));
        }
        curl_easy_cleanup(curlHandler);
    }

    if (server) {
        server->StopServer();
    }
    if (server_thread) {
        server_thread->join();
    }
    server.reset();
}

INSTANTIATE_TEST_SUITE_P(
        HttpTokenServiceTest,
        THttpServerFixture,
        ::testing::Values(
                THttpTestData {
                    CURRENT_USER
                },
                THttpTestData {
                    "UnknownUser"
                }));
