#pragma once

#include "credentials.h"
#include "resource.h"
#include "subject.h"

#include <chrono>
#include <future>
#include <utility>
#include <vector>

namespace yandex::cloud::auth {

    struct AccessServiceClientConfig {
        // Should be "service/version", example: "iam/1.0".
        std::string ClientName;
        // Should be valid GRPC endpoint, example: "localhost:4286".
        std::string Endpoint;
        // After a duration of this time the client pings server to see if the transport is still alive.
        std::chrono::duration<double> KeepAliveTime;
        // After waiting for a duration of this time, if the keepalive ping sender does
        // not receive the ping ack, it will close the transport.
        std::chrono::duration<double> KeepAliveTimeout;
        // True for plaintext connections.
        bool Plaintext;
        struct RetryPolicy {
            // Max retry attempts. Zero means never retry. Default is 7 retries.
            int MaxAttempts = 7;
            // Initial backoff to retry the failed call. Default is 0.1 second.
            std::chrono::duration<double> InitialBackOff = std::chrono::milliseconds(100);
            // Maximal backoff to retry the failed call. Default is 2 second.
            std::chrono::duration<double> MaxBackOff = std::chrono::seconds(2);
            // Step to increase the backoff interval. Default is 1.6.
            double BackoffMultiplier = 1.6;
            // GRPC status codes to retry, in std::string form. Default is
            //      ABORTED, CANCELLED, DEADLINE_EXCEEDED, INTERNAL, UNAVAILABLE, UNKNOWN.
            std::vector<std::string> RetryableStatusCodes;
        } RetryPolicy;
        std::string RootCertificate;
        // Override the target name used for SSL host name checking. Ignored if Plaintext is true.
        std::string SslTargetNameOverride;
    };

    enum AuthStatus {
        ERROR,
        OK,
        UNAUTHENTICATED,
        PERMISSION_DENIED,
    };

    class Error {
    public:
        Error(int32_t code, std::string message, std::string details)
            : Code_(code)
            , Message_(std::move(message))
            , Details_(std::move(details))
        {
        }

        bool operator==(const Error& other) const {
            return Code() == other.Code() && Message() == other.Message() && Details() == other.Details();
        }

        int32_t Code() const {
            return Code_;
        }

        const std::string& Message() const {
            return Message_;
        }

        const std::string& Details() const {
            return Details_;
        }

    private:
        int32_t Code_;
        std::string Message_;
        std::string Details_;
    };

    std::ostream& operator<<(std::ostream& out, const Error& error);

    class AuthenticationResult {
    public:
        AuthenticationResult(Error error, Subject subject);

        AuthenticationResult(AuthStatus status, Subject subject);

        explicit operator bool() const {
            return Status_ == AuthStatus::OK;
        }

        bool operator==(const AuthenticationResult& other) const {
            return GetStatus() == other.GetStatus() && GetSubject() == other.GetSubject() && GetError() == other.GetError();
        }

        AuthStatus GetStatus() const {
            return Status_;
        }

        const Subject& GetSubject() const {
            return Subject_;
        }

        const Error& GetError() const {
            return Error_;
        }

    private:
        AuthStatus Status_;
        yandex::cloud::auth::Subject Subject_;
        yandex::cloud::auth::Error Error_;
    };

    std::ostream& operator<<(std::ostream& out, const AuthenticationResult& result);

    class AuthorizationResult {
    public:
        AuthorizationResult(Error error, Subject subject);

        AuthorizationResult(AuthStatus status, Subject subject);

        explicit operator bool() const {
            return Status_ == AuthStatus::OK;
        }

        bool operator==(const AuthorizationResult& other) const {
            return GetStatus() == other.GetStatus() && GetSubject() == other.GetSubject() &&
                   GetError() == other.GetError();
        }

        AuthStatus GetStatus() const {
            return Status_;
        }

        const Subject& GetSubject() const {
            return Subject_;
        }

        const Error& GetError() const {
            return Error_;
        }

    private:
        AuthStatus Status_;
        yandex::cloud::auth::Subject Subject_;
        yandex::cloud::auth::Error Error_;
    };

    std::ostream& operator<<(std::ostream& out, const AuthorizationResult& result);

    class AccessServiceClient {
    public:
        class ImplementationDetails;
        explicit AccessServiceClient(std::shared_ptr<ImplementationDetails> implementation_details);

        virtual ~AccessServiceClient();

        AuthenticationResult Authenticate(const Credentials& credentials);

        AuthorizationResult Authorize(
            const Credentials& credentials,
            const std::string& permission,
            const Resource& resource) {
            return Authorize(credentials, permission, {resource});
        }

        AuthorizationResult Authorize(
            const Credentials& credentials,
            const std::string& permission,
            const std::initializer_list<Resource>& resource_path);

        static std::shared_ptr<AccessServiceClient> Create(
            const AccessServiceClientConfig& config);

    private:
        std::shared_ptr<ImplementationDetails> Private_;
    };

    class AccessServiceAsyncClient {
    public:
        class ImplementationDetails;
        explicit AccessServiceAsyncClient(std::shared_ptr<ImplementationDetails> implementationDetails);

        virtual ~AccessServiceAsyncClient();

        std::future<AuthenticationResult> Authenticate(const Credentials& credentials);

        void Authenticate(const Credentials& credentials, std::function<void(const AuthenticationResult&)> callback);

        std::future<AuthorizationResult> Authorize(
            const Credentials& credentials,
            const std::string& permission,
            const Resource& resource) {
            return Authorize(credentials, permission, {resource});
        }

        std::future<AuthorizationResult> Authorize(
            const Credentials& credentials,
            const std::string& permission,
            const std::initializer_list<Resource>& resource_path);

        void Authorize(
            const Credentials& credentials,
            const std::string& permission,
            const Resource& resource,
            std::function<void(const AuthorizationResult&)> callback) {
            Authorize(credentials, permission, {resource}, std::move(callback));
        }

        void Authorize(
            const Credentials& credentials,
            const std::string& permission,
            const std::initializer_list<Resource>& resource_path,
            std::function<void(const AuthorizationResult&)> callback);

        static std::shared_ptr<AccessServiceAsyncClient> Create(
            const AccessServiceClientConfig& config);

    private:
        std::shared_ptr<ImplementationDetails> Private_;
    };

}
