#include "cloud-auth/client.h"
#include "convert.h"
#include "internal.h"

#include <grpcpp/grpcpp.h>
#include <yandex/cloud/priv/accessservice/v2/access_service.grpc.pb.h>

#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <random>
#include <sys/stat.h>
#include <utility>

namespace yandex::cloud::auth {

    static std::default_random_engine RANDOM_GENERATOR;

    static const std::vector<grpc::StatusCode> DEFAULT_RETRYABLE_CODES{
        grpc::StatusCode::ABORTED,
        grpc::StatusCode::CANCELLED,
        grpc::StatusCode::DEADLINE_EXCEEDED,
        grpc::StatusCode::INTERNAL,
        grpc::StatusCode::UNAVAILABLE,
        grpc::StatusCode::UNKNOWN};

    std::ostream& operator<<(std::ostream& out, const Error& error)
    {
        return out << "Error {code=" << error.Code()
                   << ", message='" << error.Message()
                   << "', details='" << error.Details()
                   << "'}";
    }

    std::ostream& operator<<(std::ostream& out, const AuthenticationResult& result)
    {
        return out << "AuthenticationResult {status=" << result.GetStatus()
                   << ", subject=" << result.GetSubject()
                   << ", error=" << result.GetError()
                   << "}";
    }

    std::ostream& operator<<(std::ostream& out, const AuthorizationResult& result)
    {
        return out << "AuthorizationResult {status=" << result.GetStatus()
                   << ", subject=" << result.GetSubject()
                   << ", error=" << result.GetError();
    }

    AuthenticationResult::AuthenticationResult(auth::Error error, auth::Subject subject)
        : Status_(convert::ToStatus(grpc::StatusCode(error.Code())))
        , Subject_(std::move(subject))
        , Error_(std::move(error))
    {
    }

    AuthenticationResult::AuthenticationResult(auth::AuthStatus status, auth::Subject subject)
        : Status_(status)
        , Subject_(std::move(subject))
        , Error_(grpc::OK, "", "")
    {
    }

    AuthorizationResult::AuthorizationResult(auth::Error error, auth::Subject subject)
        : Status_(convert::ToStatus(grpc::StatusCode(error.Code())))
        , Subject_(std::move(subject))
        , Error_(std::move(error))
    {
    }

    AuthorizationResult::AuthorizationResult(auth::AuthStatus status, auth::Subject subject)
        : Status_(status)
        , Subject_(std::move(subject))
        , Error_(grpc::OK, "", "")
    {
    }

    class AbstractAsyncCall {
    public:
        virtual ~AbstractAsyncCall() = default;

        virtual void Invoke(const std::shared_ptr<yandex::cloud::priv::accessservice::v2::AccessService::Stub>& stub,
                            const std::shared_ptr<grpc::CompletionQueue>& queue) = 0;

        virtual void Success() = 0;

        virtual void Failure() = 0;

        virtual std::string GetTypeName() = 0;

        void Log() {
            gpr_log(GPR_INFO, "RPC %20s#%d: (%d %s)",
                    GetTypeName().c_str(), RetryAttempt_, Status_.error_code(), Status_.error_message().c_str());
        }

        bool CanRetry(
            int max_attempts,
            const std::vector<grpc::StatusCode>& retryable_status_codes)
        {
            bool result;
            if (++RetryAttempt_ > max_attempts) {
                result = false;
            } else {
                auto iter = std::find(std::begin(retryable_status_codes), std::end(retryable_status_codes), Status_.error_code());
                result = iter != std::end(retryable_status_codes);
            }
            return result;
        }

        grpc::Status Status() const {
            return Status_;
        }

        int RetryAttempt() const {
            return RetryAttempt_;
        }

    protected:
        // Storage for the status of the RPC upon completion.
        grpc::Status Status_{};
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        std::unique_ptr<grpc::ClientContext> Context_;

    private:
        int RetryAttempt_{0};
    };

    template <typename DERIVED, typename REQUEST, typename REPLY, typename RESULT>
    class AbstractAsyncCallT: public AbstractAsyncCall {
    protected:
        using Finalizer = std::variant<std::promise<RESULT>, std::function<void(RESULT)>>;

        explicit AbstractAsyncCallT(REQUEST request, Finalizer finalizer)
            : Request_(std::move(request))
            , Finalizer_(std::move(finalizer))
        {
        }

        struct FinalizerVisitor {
            const RESULT& result;

            void operator()(std::promise<RESULT>& promise) {
                promise.set_value(result);
            }

            void operator()(std::function<void(RESULT)>& callback) {
                callback(result);
            }
        };

        void Success() override {
            const auto& result = static_cast<DERIVED*>(this)->Convert(Reply_);
            std::visit(FinalizerVisitor{result}, Finalizer_);
        }

        void Failure() override {
            const auto& result = convert::ToError<RESULT>(Status_);
            std::visit(FinalizerVisitor{result}, Finalizer_);
        }

        const REQUEST& GetRequest() const {
            return Request_;
        }

        REPLY& GetReply() {
            return Reply_;
        }

    public:
        std::string GetTypeName() override {
            return Request_.GetDescriptor()->name();
        }

        std::future<RESULT> GetFuture() {
            return std::get_if<std::promise<RESULT>>(&Finalizer_)->get_future();
        }

    private:
        // Container for the data we (re)send the server.
        REQUEST Request_;

        // Container for the data we expect from the server.
        REPLY Reply_;

        Finalizer Finalizer_;
    };

    class AsyncAuthenticateCall: public AbstractAsyncCallT<
                                     AsyncAuthenticateCall,
                                     yandex::cloud::priv::accessservice::v2::AuthenticateRequest,
                                     yandex::cloud::priv::accessservice::v2::AuthenticateResponse,
                                     AuthenticationResult> {
    public:
        explicit AsyncAuthenticateCall(
            yandex::cloud::priv::accessservice::v2::AuthenticateRequest request)
            : AbstractAsyncCallT(std::move(request), std::promise<AuthenticationResult>())
        {
        }

        explicit AsyncAuthenticateCall(
            yandex::cloud::priv::accessservice::v2::AuthenticateRequest request,
            std::function<void(AuthenticationResult)> callback)
            : AbstractAsyncCallT(std::move(request), callback)
        {
        }

        void Invoke(const std::shared_ptr<yandex::cloud::priv::accessservice::v2::AccessService::Stub>& stub,
                    const std::shared_ptr<grpc::CompletionQueue>& queue) override {
            Context_ = std::make_unique<grpc::ClientContext>();
            auto reader = stub->AsyncAuthenticate(Context_.get(), GetRequest(), queue.get());
            reader->Finish(&GetReply(), &Status_, this);
        }

        static AuthenticationResult Convert(const yandex::cloud::priv::accessservice::v2::AuthenticateResponse& response) {
            return convert::ToAuthenticationResult(AuthStatus::OK, response);
        }
    };

    class AsyncAuthorizeCall: public AbstractAsyncCallT<
                                  AsyncAuthorizeCall,
                                  yandex::cloud::priv::accessservice::v2::AuthorizeRequest,
                                  yandex::cloud::priv::accessservice::v2::AuthorizeResponse,
                                  AuthorizationResult> {
    public:
        explicit AsyncAuthorizeCall(
            yandex::cloud::priv::accessservice::v2::AuthorizeRequest request)
            : AbstractAsyncCallT(std::move(request), std::promise<AuthorizationResult>())
        {
        }

        explicit AsyncAuthorizeCall(
            yandex::cloud::priv::accessservice::v2::AuthorizeRequest request,
            std::function<void(AuthorizationResult)> callback)
            : AbstractAsyncCallT(std::move(request), callback)
        {
        }

        void Invoke(const std::shared_ptr<yandex::cloud::priv::accessservice::v2::AccessService::Stub>& stub,
                    const std::shared_ptr<grpc::CompletionQueue>& queue) override {
            Context_ = std::make_unique<grpc::ClientContext>();
            auto reader = stub->AsyncAuthorize(Context_.get(), GetRequest(), queue.get());
            reader->Finish(&GetReply(), &Status_, this);
        }

        static AuthorizationResult Convert(const yandex::cloud::priv::accessservice::v2::AuthorizeResponse& response) {
            return convert::ToAuthorizationResult(AuthStatus::OK, response);
        }
    };

    class AccessServiceAsyncClient::ImplementationDetails {
    public:
        ImplementationDetails(
            std::shared_ptr<grpc::Channel> channel,
            AccessServiceClientConfig config)
            : CompletionQueue_(std::make_shared<grpc::CompletionQueue>())
            , Stub_(yandex::cloud::priv::accessservice::v2::AccessService::NewStub(std::move(channel)))
            // Spawn reader thread that loops indefinitely
            , Thread_(std::thread(&ImplementationDetails::AsyncCompleteRpc, this))
            , Config_(std::move(config))
            , RetryableStatusCodes_(convert::ParseStatusCodes(Config_.RetryPolicy.RetryableStatusCodes, DEFAULT_RETRYABLE_CODES))
        {
            ShutdownMutex_.lock();
        }

        // Loop while listening for completed responses.
        void AsyncCompleteRpc() {
            AbstractAsyncCall* call;
            bool ok{};

            // Block until the next result is available in the completion queue "cq".
            while (CompletionQueue_->Next(reinterpret_cast<void**>(&call), &ok)) {
                // Verify that the Request_ was completed successfully. Note that "ok"
                // corresponds solely to the Request_ for updates introduced by Finish().
                GPR_ASSERT(ok);

                process(call);
            }
        }

        void process(
            AbstractAsyncCall* call)
        {
            call->Log();
            if (call->Status().ok()) {
                call->Success();
            } else {
                if (call->CanRetry(Config_.RetryPolicy.MaxAttempts, RetryableStatusCodes_)) {
                    auto backoff = Config_.RetryPolicy.InitialBackOff *
                                   std::pow(Config_.RetryPolicy.BackoffMultiplier, call->RetryAttempt());
                    // Do not retry if the shutdown is initiated.
                    if (!ShutdownMutex_.try_lock_for(std::min(backoff, Config_.RetryPolicy.MaxBackOff))) {
                        call->Invoke(Stub_, CompletionQueue_);
                        return;
                    }
                }
                call->Failure();
            }

            // Once we're complete, deallocate the call object.
            delete call;
        }

        std::timed_mutex ShutdownMutex_;
        std::shared_ptr<grpc::CompletionQueue> CompletionQueue_;
        std::shared_ptr<yandex::cloud::priv::accessservice::v2::AccessService::Stub> Stub_;
        std::thread Thread_;
        AccessServiceClientConfig Config_;
        std::vector<grpc::StatusCode> RetryableStatusCodes_;
    };

    AccessServiceAsyncClient::AccessServiceAsyncClient(
        std::shared_ptr<AccessServiceAsyncClient::ImplementationDetails> implementationDetails)
        : Private_(std::move(implementationDetails))
    {
    }

    AccessServiceAsyncClient::~AccessServiceAsyncClient() {
        Private_->ShutdownMutex_.unlock();
        Private_->CompletionQueue_->Shutdown();
        Private_->Thread_.join();
    }

    std::future<AuthenticationResult> AccessServiceAsyncClient::Authenticate(const Credentials& credentials) {
        auto call = new AsyncAuthenticateCall(convert::ToAuthenticateRequest(credentials));
        call->Invoke(Private_->Stub_, Private_->CompletionQueue_);
        return call->GetFuture();
    }

    void AccessServiceAsyncClient::Authenticate(
        const Credentials& credentials,
        std::function<void(const AuthenticationResult&)> callback)
    {
        auto call = new AsyncAuthenticateCall(convert::ToAuthenticateRequest(credentials), std::move(callback));
        call->Invoke(Private_->Stub_, Private_->CompletionQueue_);
    }

    std::future<AuthorizationResult>
    AccessServiceAsyncClient::Authorize(
        const Credentials& credentials, const std::string& permission,
        const std::initializer_list<Resource>& resource_path)
    {
        auto call = new AsyncAuthorizeCall(convert::ToAuthorizeRequest(credentials, permission, resource_path));
        call->Invoke(Private_->Stub_, Private_->CompletionQueue_);
        return call->GetFuture();
    }

    void AccessServiceAsyncClient::Authorize(
        const Credentials& credentials, const std::string& permission,
        const std::initializer_list<Resource>& resource_path,
        std::function<void(const AuthorizationResult&)> callback)
    {
        auto call = new AsyncAuthorizeCall(convert::ToAuthorizeRequest(credentials, permission, resource_path), std::move(callback));
        call->Invoke(Private_->Stub_, Private_->CompletionQueue_);
    }

    std::shared_ptr<AccessServiceAsyncClient> AccessServiceAsyncClient::Create(
        const AccessServiceClientConfig& config)
    {
        std::shared_ptr<grpc::Channel> channel = internal::CreateChannel(config);
        return internal::CreateAccessServiceAsyncClient(internal::CreateChannel(config), config);
    }

    class AccessServiceClient::ImplementationDetails {
    public:
        ImplementationDetails(
            std::shared_ptr<grpc::Channel> channel,
            AccessServiceClientConfig config)
            : Stub_(yandex::cloud::priv::accessservice::v2::AccessService::NewStub(std::move(channel)))
            , Config_(std::move(config))
            , RetryableStatusCodes_(convert::ParseStatusCodes(Config_.RetryPolicy.RetryableStatusCodes, DEFAULT_RETRYABLE_CODES))
        {
            ShutdownMutex_.lock();
        }

        bool IsRetryable(int retry, const grpc::Status& status) const {
            bool result;
            if (retry > Config_.RetryPolicy.MaxAttempts) {
                result = false;
            } else {
                auto iter = std::find(std::begin(RetryableStatusCodes_), std::end(RetryableStatusCodes_), status.error_code());
                result = iter != std::end(RetryableStatusCodes_);
            }
            return result;
        }

        template <class GRPC_REQUEST, class GRPC_RESPONSE, class RESULT>
        RESULT SendRequest(
            const GRPC_REQUEST& request,
            std::function<grpc::Status(
                std::shared_ptr<yandex::cloud::priv::accessservice::v2::AccessService::Stub>,
                grpc::ClientContext*,
                const GRPC_REQUEST&,
                GRPC_RESPONSE*)>
                invoke,
            std::function<RESULT(AuthStatus, const GRPC_RESPONSE&)> convert)
        {
            GRPC_RESPONSE response;
            grpc::Status status;
            int retry = 0;
            const auto& policy = Config_.RetryPolicy;
            std::chrono::duration<double> backoff = policy.InitialBackOff;
            while (true) {
                grpc::ClientContext context;
                status = invoke(Stub_, &context, request, &response);
                gpr_log(GPR_INFO, "RPC %20s#%d: (%d %s)",
                        request.descriptor()->name().c_str(), retry, status.error_code(), status.error_message().c_str());
                if (status.ok()) {
                    return convert(AuthStatus::OK, response);
                }
                if (!IsRetryable(retry++, status)) {
                    break;
                }

                std::uniform_real_distribution<> distribution(0.0, backoff.count());
                if (ShutdownMutex_.try_lock_for(std::chrono::duration<double>(distribution(RANDOM_GENERATOR)))) {
                    break;
                }
                backoff = std::min(backoff * policy.BackoffMultiplier, policy.MaxBackOff);
            }
            return convert::ToError<RESULT>(status);
        }

        void Shutdown() {
            ShutdownMutex_.unlock();
        }

    private:
        std::timed_mutex ShutdownMutex_;
        std::shared_ptr<yandex::cloud::priv::accessservice::v2::AccessService::Stub> Stub_;
        AccessServiceClientConfig Config_;
        std::vector<grpc::StatusCode> RetryableStatusCodes_;
    };

    AccessServiceClient::AccessServiceClient(
        std::shared_ptr<ImplementationDetails> implementation_details)
        : Private_(std::move(implementation_details))
    {
    }

    AccessServiceClient::~AccessServiceClient() {
        Private_->Shutdown();
    }

    AuthenticationResult AccessServiceClient::Authenticate(
        const Credentials& credentials)
    {
        const auto& request = convert::ToAuthenticateRequest(credentials);
        return Private_->SendRequest<
            yandex::cloud::priv::accessservice::v2::AuthenticateRequest,
            yandex::cloud::priv::accessservice::v2::AuthenticateResponse,
            AuthenticationResult>(
            request,
            &yandex::cloud::priv::accessservice::v2::AccessService::Stub::Authenticate,
            convert::ToAuthenticationResult);
    }

    AuthorizationResult AccessServiceClient::Authorize(
        const Credentials& credentials,
        const std::string& permission,
        const std::initializer_list<Resource>& resource_path)
    {
        const auto& request = convert::ToAuthorizeRequest(credentials, permission, resource_path);
        return Private_->SendRequest<
            yandex::cloud::priv::accessservice::v2::AuthorizeRequest,
            yandex::cloud::priv::accessservice::v2::AuthorizeResponse,
            AuthorizationResult>(
            request,
            &yandex::cloud::priv::accessservice::v2::AccessService::Stub::Authorize,
            convert::ToAuthorizationResult);
    }

    std::shared_ptr<AccessServiceClient> AccessServiceClient::Create(
        const AccessServiceClientConfig& config)
    {
        return internal::CreateAccessServiceClient(internal::CreateChannel(config), config);
    }

    namespace internal {

        std::string GetExecutableName() {
            std::string executable_name;
            std::ifstream stm("/proc/self/comm");
            std::getline(stm, executable_name);
            return executable_name;
        }

        std::string GetExecutableModificationDate() {
            struct stat executable_stat {};
            auto executable_path = std::filesystem::read_symlink("/proc/self/exe");
            ::stat(executable_path.c_str(), &executable_stat);
            auto tm = ::gmtime(&executable_stat.st_mtim.tv_sec);
            return std::to_string(tm->tm_year + 1900) + std::to_string(tm->tm_mon + 1) + std::to_string(tm->tm_mday);
        }

        std::shared_ptr<grpc::Channel> CreateChannel(
            const AccessServiceClientConfig& config)
        {
            grpc::ChannelArguments channelArgs;
            const auto& client_name = config.ClientName.empty()
                                          ? GetExecutableName() + "/" + GetExecutableModificationDate()
                                          : config.ClientName;
            channelArgs.SetUserAgentPrefix(client_name + " " + USER_AGENT + "/" + VERSION);
            std::shared_ptr<grpc::ChannelCredentials> credentials;
            if (config.Plaintext) {
                credentials = grpc::InsecureChannelCredentials();
            } else {
                grpc::SslCredentialsOptions ssl_options;
                if (!config.RootCertificate.empty()) {
                    ssl_options.pem_root_certs = {config.RootCertificate};
                }
                credentials = grpc::SslCredentials(ssl_options);
            }
            channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS,
                               std::chrono::duration_cast<std::chrono::milliseconds>(config.KeepAliveTime).count());
            channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS,
                               std::chrono::duration_cast<std::chrono::milliseconds>(config.KeepAliveTimeout).count());
            if (!config.SslTargetNameOverride.empty()) {
                channelArgs.SetString(GRPC_SSL_TARGET_NAME_OVERRIDE_ARG,
#ifdef ARCADIA_BUILD
                    TString(config.SslTargetNameOverride)
#else
                    config.SslTargetNameOverride
#endif
                );
            }
            return std::shared_ptr<grpc::Channel>(grpc::CreateCustomChannel(
#ifdef ARCADIA_BUILD
            TString(config.Endpoint),
#else
            config.Endpoint,
#endif
            credentials, channelArgs));
        }

        std::shared_ptr<AccessServiceClient> CreateAccessServiceClient(
            std::shared_ptr<grpc::Channel> channel,
            const AccessServiceClientConfig& config)
        {
            return std::make_shared<AccessServiceClient>(
                std::make_shared<AccessServiceClient::ImplementationDetails>(std::move(channel), config));
        }

        std::shared_ptr<AccessServiceAsyncClient> CreateAccessServiceAsyncClient(
            std::shared_ptr<grpc::Channel> channel,
            const AccessServiceClientConfig& config)
        {
            return std::make_shared<AccessServiceAsyncClient>(
                std::make_shared<AccessServiceAsyncClient::ImplementationDetails>(std::move(channel), config));
        }

    }

}
