#include <server.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <pwd.h>
#include <unistd.h>
#include <sys/wait.h>

#include <contrib/libs/jwt-cpp/include/jwt-cpp/jwt.h>

#include <library/cpp/logger/global/global.h>
#include <google/protobuf/text_format.h>

#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.grpc.pb.h>

// To suppress libprotobuf warnings about invalid utf-8 strings
google::protobuf::LogSilencer log_silencer;

static std::unique_ptr<NTokenAgent::TServer> server;
static std::unique_ptr<std::thread> server_thread;
static std::filesystem::path random_folder_name;
static std::filesystem::path fuzzing_helper_path;

static std::filesystem::path GetTempFolderName() {
    if (random_folder_name.empty()) {
        // Can not use random here. All fuzzing builds should be reproducible.
        auto path = std::filesystem::temp_directory_path()
            / ("token-agent-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path);
        random_folder_name.swap(path);
    }
    return random_folder_name;
}

static std::filesystem::path GetGrpcSocketPath() {
    return GetTempFolderName() / "grpc.socket";
}

static std::filesystem::path CreateConfig() {
    auto temp_path = GetTempFolderName();
    auto config_path = temp_path / "config.yaml";
    std::ofstream(config_path)
           << "cacheUpdateInterval: 1s\n"
              "keyPath: " << temp_path / "key" << "\n"
              "cachePath: " << temp_path / "cache" << "\n"
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
    std::ofstream(roles_path / (current_user + ".yaml")) << "token: FUZZ-TOKEN\n";
    std::ofstream(current_user_role_path / "tag.yaml") << "token: TAG-FUZZ-TOKEN\n";

    return config_path;
}

static std::shared_ptr<grpc::Channel> OpenChannel() {
    grpc::ChannelArguments args;
    auto credentials = grpc::InsecureChannelCredentials();
    return grpc::CreateCustomChannel("unix://" + GetGrpcSocketPath().string(), credentials, args);
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
    bool debug = false;
    for (int i = 0; !debug && i < *argc; ++i) {
        debug = (::strcmp("-debug", (*argv)[i]) == 0);
    }
    if (debug) {
        InitGlobalLog2Console(TLOG_DEBUG);
    } else {
        InitGlobalLog2Null();
    }

    const char* fuzzing_helper_str = std::getenv("FUZZING_HELPER");
    if (!fuzzing_helper_str) {
        fuzzing_helper_str = "./test-helper";
    }
    if (std::filesystem::exists(fuzzing_helper_str)) {
        fuzzing_helper_path = fuzzing_helper_str;
        INFO_LOG << "Using " << fuzzing_helper_path;
    } else {
        INFO_LOG << "No test-helper found, use 'sudo -u ## grpcurl' instead";
    }

    auto config_path = CreateConfig();
    auto config = NTokenAgent::TConfig::FromFile(config_path);
    server = std::make_unique<NTokenAgent::TServer>(config);
    server_thread = std::make_unique<std::thread>([] {
        server->RunServer();
    });
    return 0;
}

extern "C" void LLVMFuzzerCleanup()
{
    // Shutdown the server
    if (server) {
        server->StopServer();
    }
    if (server_thread) {
        server_thread->join();
    }
    server.reset();

    // Cleanup temporary config folder
   std::filesystem::remove_all(GetTempFolderName());
   google::protobuf::ShutdownProtobufLibrary();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    int32_t uid {0};
    std::string tag;
    if (size >= sizeof(int32_t)) {
        mempcpy(&uid, data, sizeof(int32_t));
        data += sizeof(int32_t);
        size -= sizeof(int32_t);
        tag.assign((const char *) data, size);
    }

    grpc::ClientContext context;
    yandex::cloud::priv::iam::v1::GetTokenRequest request;
    yandex::cloud::priv::iam::v1::GetTokenResponse reply;

    request.mutable_tag()->assign(tag);
    if (uid) {
        // Switch to another user
        auto pid = fork();
        DEBUG_LOG << "Fork pid " << pid << "\n";
        if (pid == 0) {
            // Child part of the fork
            if (fuzzing_helper_path.empty()) {
                tag.erase(std::remove(tag.begin(), tag.end(), '"'), tag.end());
                auto str_uid = "#" + std::to_string(uid);
                auto ret_value = execl("/bin/sudo", "/bin/sudo",
                      "--non-interactive",
                      "--user", str_uid.c_str(),
                      "/home/pbludov/bin/grpcurl", "--plaintext",
                      "-d", ("{\"tag\": \"" + tag + "\"}").c_str(),
                      "--unix", GetGrpcSocketPath().c_str(),
                      "yandex.cloud.priv.iam.v1.TokenAgent/GetToken",
                      nullptr);
                DEBUG_LOG << "/bin/sudo -u " << str_uid.c_str()
                    << " grpcurl tag: " << tag.c_str()
                    << " ret: " << ret_value << "\n";
            } else {
                auto encoded_tag = jwt::base::encode<jwt::alphabet::base64url>(tag);
                auto ret_value = execl(fuzzing_helper_path.c_str(),
                      fuzzing_helper_path.c_str(),
                      GetGrpcSocketPath().c_str(),
                      "-u", std::to_string(uid).c_str(),
                      "-t", encoded_tag.c_str(),
                      nullptr);
                DEBUG_LOG << fuzzing_helper_path.c_str()
                    << " tag: " << tag.c_str()
                    << " ret: " << ret_value
                    << "\n";
            }
        } else {
            // Parent part of the fork
            siginfo_t info{};
            DEBUG_LOG << "waiting for child process " << pid << " to terminate\n";
            auto ret = ::waitid(P_PID, pid, &info, WEXITED);
            DEBUG_LOG << "Process " << pid << " terminated, ret: " << ret
                << " status: " << info.si_status << "\n";
        }
    } else {
        // Invoke for the current user
        auto stub = yandex::cloud::priv::iam::v1::TokenAgent::NewStub(OpenChannel());
        request.mutable_tag()->assign(tag);
        grpc::Status status = stub->GetToken(&context, request, &reply);
        DEBUG_LOG << "GetToken result: status " << (int) status.error_code() << ", token: " << reply.iam_token() << "\n" << "tag " << tag;
    }

    return 0;
}

#ifdef FUZZING_DEBUG

int main(int argc, char** argv) {

    LLVMFuzzerInitialize(&argc, &argv);

    for (int i = 1; i < argc; ++i) {
        LLVMFuzzerTestOneInput((const uint8_t *) argv[i], strlen(argv[i]));
    }

    uint8_t buf[7] = {0, 0, 0, 0, 't', 'a', 'g'};
    for (int i = 0; i < 10000; ++i) {
        buf[3] = i;
        LLVMFuzzerTestOneInput(buf, sizeof(buf));
    }

    LLVMFuzzerCleanup();
    return 0;
}

#endif
