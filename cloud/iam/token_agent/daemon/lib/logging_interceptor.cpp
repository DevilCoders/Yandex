#include <sys/socket.h>
#include <library/cpp/logger/global/global.h>

#ifdef ARCADIA_BUILD
    #include <contrib/libs/grpc/include/grpcpp/impl/codegen/server_context.h>
#else
    #include <contrib/libs/grpc/include/grpcpp/impl/codegen/server_context_impl.h>
#endif

#include <cloud/bitbucket/private-api/yandex/cloud/priv/sensitive.pb.h>
#include "logging_interceptor.h"

namespace NTokenAgent {
    static TLog ACCESS_LOG;

    static bool ContainsSensitiveFields(const google::protobuf::Message* message) {
        const auto* descriptor = message->GetDescriptor();
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const auto& options = descriptor->field(i)->options();
            if (options.HasExtension(yandex::cloud::priv::sensitive)) {
                return true;
            }
        }

        return false;
    }

    static bool GetPeerCredentials(const std::string& peer, ucred& peercred) {
        if (peer.find("fd:") == 0) {
            auto fd = std::stoi(peer.substr(3));

            socklen_t peercred_len = sizeof(peercred);
            return ::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peercred, &peercred_len) == 0;
        }
        return false;
    }

    LoggingInterceptor::LoggingInterceptor(grpc::experimental::ServerRpcInfo* info)
        : Info(info)
        , MessageSize(0)
        , RequestId("-")
    {
    }

    void LoggingInterceptor::Intercept(grpc::experimental::InterceptorBatchMethods* methods) {
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA)) {
            auto map = methods->GetRecvInitialMetadata();
            if (map == nullptr) {
                ERROR_LOG << "Failed to get initial metadata for request" << Info->method();
            } else {
                auto it = map->find("x-request-id");
                if (it != map->end()) {
                    RequestId = std::string(it->second.cbegin(), it->second.cend());
                }
            }
        }

        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_MESSAGE)) {
            auto request = static_cast<google::protobuf::Message*>(methods->GetRecvMessage());
            if (request != nullptr) {
                if (ContainsSensitiveFields(request)) {
                    RequestMessage = "***";
                } else if (request->ByteSize() == 0) {
                    RequestMessage = "-";
                } else {
                    RequestMessage = request->ShortDebugString();
                }
            }
        }

        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) {
            auto buffer = methods->GetSerializedSendMessage();
            if (buffer != nullptr) {
                MessageSize = buffer->Length();
            }
        }

        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_STATUS)) {
            auto status = methods->GetSendStatus();

            ucred peercred{};
            GetPeerCredentials(Info->server_context()->peer(), peercred);

            ACCESS_LOG << TInstant::Now()
                       << " | " << peercred.uid
                       << " | " << peercred.gid
                       << " | " << peercred.pid
                       << " | " << Info->method()
                       << " | " << RequestMessage
                       << " | " << RequestId
                       << " | " << std::to_string(status.error_code())
                       << " | " << MessageSize
                       << "\n";
        }

        methods->Proceed();
    }

    void LoggingInterceptor::Init(THolder<TLogBackend> backend) {
        ACCESS_LOG.ResetBackend(std::move(backend));
    }
}
