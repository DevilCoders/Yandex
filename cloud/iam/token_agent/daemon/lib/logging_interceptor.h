#pragma once

#include <grpcpp/impl/codegen/interceptor.h>
#include <grpcpp/impl/codegen/server_interceptor.h>

#include <util/generic/ptr.h>

#include <library/cpp/logger/backend.h>

namespace NTokenAgent {
    class LoggingInterceptor: public grpc::experimental::Interceptor {
    public:
        explicit LoggingInterceptor(grpc::experimental::ServerRpcInfo* info);
        void Intercept(grpc::experimental::InterceptorBatchMethods* methods) override;
        static void Init(THolder<TLogBackend> backend);

    private:
        grpc::experimental::ServerRpcInfo* Info;
        size_t MessageSize;
        std::string RequestId;
        std::string RequestMessage;
    };

    class LoggingInterceptorFactory
        : public grpc::experimental::ServerInterceptorFactoryInterface {
    public:
        grpc::experimental::Interceptor* CreateServerInterceptor(
            grpc::experimental::ServerRpcInfo* info) override {
            return new LoggingInterceptor(info);
        }
    };
}
