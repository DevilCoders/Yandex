#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.pb.h>
#include <cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token_agent.grpc.pb.h>

#include <contrib/libs/jwt-cpp/include/jwt-cpp/jwt.h>
#include <grpcpp/grpcpp.h>

#include <library/cpp/getopt/last_getopt.h>

const std::string DEFAULT_EXPECTED;
const std::string DEFAULT_TAG;
constexpr int DEFAULT_UID = {0};
constexpr int DEFAULT_REPEAT = {1};

int Test(const std::string& socket_path,
         const std::string& expected,
         int repeat,
         int uid,
         const std::string& tag)
{
    grpc::Status status;
    yandex::cloud::priv::iam::v1::GetTokenRequest request;

    if (uid != 0)
    {
        if (setuid(uid))
        {
            std::cerr << "setuid failed: " << std::strerror(errno) << std::endl;
            return -1;
        }
    }

    if (!tag.empty())
    {
        // The client may crash here
        auto decoded = jwt::base::decode<jwt::alphabet::base64url>(tag);
        request.mutable_tag()->assign(decoded);
    }

    for (int n = 0; n < repeat; ++n)
    {
        grpc::ChannelArguments args;
        auto credentials = grpc::InsecureChannelCredentials();
        auto channel = grpc::CreateCustomChannel(socket_path.c_str(), credentials, args);
        auto stub = yandex::cloud::priv::iam::v1::TokenAgent::NewStub(channel);

        grpc::ClientContext context;
        yandex::cloud::priv::iam::v1::GetTokenResponse reply;

        status = stub->GetToken(&context, request, &reply);

        if (!expected.empty() && expected != reply.iam_token())
        {
            std::cerr << n
                << ": bad IAM token! Expected `" << expected
                << "` got `" << reply.iam_token() << "'." << std::endl;
        }
    }

    // return the last response code
    return (int)status.error_code();
}

int main(int argc, char** argv)
{
    // To suppress libprotobuf warnings about invalid utf-8 strings
    google::protobuf::LogSilencer log_silencer;

    int ret{};
    std::string socket_path;
    try {
        NLastGetopt::TOpts opts;
        NLastGetopt::TOpt& opt_expected =
            opts.AddLongOption('e', "expected", "expected IAM token")
                .DefaultValue(DEFAULT_EXPECTED);
        NLastGetopt::TOpt& opt_repeat =
            opts.AddLongOption('r', "repeat", "number of repetitions")
                .DefaultValue(DEFAULT_REPEAT);
        NLastGetopt::TOpt& opt_tag =
            opts.AddLongOption('t', "tag", "tag for user")
                .DefaultValue(DEFAULT_TAG);
        NLastGetopt::TOpt& opt_uid =
            opts.AddLongOption('u', "user-uid", "UID of user to impersonate")
                .DefaultValue(DEFAULT_UID);
        opts.AddFreeArgBinding("socket", socket_path, "UDS socket path");
        opts.SetFreeArgsMax(1);
        opts.AddHelpOption();
        opts.AddVersionOption();
        THolder<NLastGetopt::TOptsParseResult> r;
        r.Reset(new NLastGetopt::TOptsParseResult(&opts, argc, argv));
        ret = Test("unix://" + socket_path,
                   r->Get(&opt_expected),
                   r->Get<int>(&opt_repeat),
                   r->Get<int>(&opt_uid),
                   r->Get(&opt_tag));
    } catch (const std::exception& ex) {
        std::cerr << "Unhandled exception in main: " << ex.what();
        return -1;
    }
    google::protobuf::ShutdownProtobufLibrary();
    return ret;
}
