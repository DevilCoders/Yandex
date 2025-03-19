#include <library/cpp/logger/global/global.h>

#include "iam_token_client_utils.h"
#include "server.h"

namespace NTokenAgent {
    std::string TIamTokenClientUtils::ReadPassword() {
        std::string password;
        std::ifstream stream(PASSWORD_FILE);
        if (stream.good()) {
            std::getline(stream, password);
        }
        return password;
    }

    std::string TIamTokenClientUtils::ReadFile(const std::string& path)
    {
        std::ifstream stream(path);
        if (!stream) {
            ythrow TSystemError() << "Failed to read `" << path << "'.";
        }
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }

    std::shared_ptr<grpc::ChannelCredentials> TIamTokenClientUtils::CreateCredentials(const TEndpointConfig& endpoint) {
        if (endpoint.IsUseTls()) {
            grpc::SslCredentialsOptions ssl_options;
            if (!endpoint.GetRootCertificatePath().empty()) {
                ssl_options.pem_root_certs = ReadFile(endpoint.GetRootCertificatePath());
            }
            return grpc::SslCredentials(ssl_options);
        }

        return grpc::InsecureChannelCredentials();
    }

    std::shared_ptr<grpc::Channel> TIamTokenClientUtils::CreateChannel(const TEndpointConfig& endpoint) {
        DEBUG_LOG << "Creating channel to " << endpoint.GetAddress() << "\n";

        grpc::ChannelArguments args;
        if (endpoint.IsInproc()) {
            return InProcessChannel(args);
        }

        args.SetUserAgentPrefix(USER_AGENT_STRING);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 120000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, true);

        auto credentials = CreateCredentials(endpoint);
        return grpc::CreateCustomChannel(
#ifdef ARCADIA_BUILD
            TString(endpoint.GetAddress()),
#else
            endpoint.GetAddress(),
#endif
            credentials, args);
    }
}
