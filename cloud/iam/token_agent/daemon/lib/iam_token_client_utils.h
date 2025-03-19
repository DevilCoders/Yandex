#pragma once
#include <chrono>
#include <fstream>
#include <iosfwd>
#include <sstream>
#include <thread>

#include <contrib/libs/uuid/uuid.h>

#include "config.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>

namespace NTokenAgent {
    static const grpc::string USER_AGENT_STRING("yc-token-agent");
    static const grpc::string PASSWORD_FILE("/var/lib/yc/token-agent/key");

    inline grpc::string GenerateRequestId() {
        // 16 bytes in hex + 4 '-' = 36
        const int PRINTABLE_UUID_LEN = 16 * 2 + 4;
        grpc::string buffer(PRINTABLE_UUID_LEN, '\x00');
        uuid_t uuid{};

        uuid_generate_random(uuid);
        uuid_unparse(uuid, buffer.begin());

        return buffer;
    }

    class TIamTokenClientUtils {
    public:
        static std::string ReadPassword();
        static std::string ReadFile(const std::string& path);
        static std::shared_ptr<grpc::ChannelCredentials> CreateCredentials(const TEndpointConfig& endpoint);
        static std::shared_ptr<grpc::Channel> CreateChannel(const TEndpointConfig& endpoint);
    };
}
