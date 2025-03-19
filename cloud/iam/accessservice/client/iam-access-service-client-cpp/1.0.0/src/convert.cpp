#include "cloud-auth/credentials.h"
#include "convert.h"

#include <google/rpc/error_details.pb.h>
#include <google/rpc/status.pb.h>

#include <vector>

namespace yandex::cloud::auth::convert {

    static const char* GRPC_ERROR_CODES[] = {
        "OK",
        "CANCELLED",
        "UNKNOWN",
        "INVALID_ARGUMENT",
        "DEADLINE_EXCEEDED",
        "NOT_FOUND",
        "ALREADY_EXISTS",
        "PERMISSION_DENIED",
        "RESOURCE_EXHAUSTED",
        "FAILED_PRECONDITION",
        "ABORTED",
        "OUT_OF_RANGE",
        "UNIMPLEMENTED",
        "INTERNAL",
        "UNAVAILABLE",
        "DATA_LOSS",
        "UNAUTHENTICATED",
    };

    grpc::StatusCode ParseStatusCode(const std::string& str) {
        auto idx = std::find(GRPC_ERROR_CODES, std::end(GRPC_ERROR_CODES), str);
        if (idx == std::end(GRPC_ERROR_CODES)) {
            return grpc::StatusCode::DO_NOT_USE;
        }
        return grpc::StatusCode(idx - GRPC_ERROR_CODES);
    }

    std::vector<grpc::StatusCode> ParseStatusCodes(
        const std::vector<std::string>& codes,
        const std::vector<grpc::StatusCode>& defaultValue) {
        std::vector<grpc::StatusCode> result;
        if (codes.empty()) {
            result = defaultValue;
        } else {
            std::transform(std::begin(codes), std::end(codes), std::back_inserter(result), ParseStatusCode);
        }
        return result;
    }

    AuthStatus ToStatus(grpc::StatusCode code) {
        if (code == grpc::OK) {
            return AuthStatus::OK;
        } else if (code == grpc::UNAUTHENTICATED) {
            return AuthStatus::UNAUTHENTICATED;
        } else if (code == grpc::PERMISSION_DENIED) {
            return AuthStatus::PERMISSION_DENIED;
        } else {
            return AuthStatus::ERROR;
        }
    }

    Subject ToSubject(const priv::accessservice::v2::Subject& subject) {
        switch (subject.type_case()) {
            case priv::accessservice::v2::Subject::kServiceAccount:
                return ServiceAccount{subject.service_account().id(), subject.service_account().folder_id()};

            case priv::accessservice::v2::Subject::kUserAccount:
                return UserAccount{subject.user_account().id(), subject.user_account().federation_id()};

            default:
                return Anonymous{};
        }
    }

    void UnpackDetails(const grpc::Status& status, std::string& internal_details, yandex::cloud::priv::accessservice::v2::Subject& subject) {
        google::rpc::Status grpc_status;
        google::rpc::DebugInfo debug_info;
        if (grpc_status.ParseFromString(status.error_details())) {
            for (const auto& detail : grpc_status.details()) {
                if (detail.UnpackTo(&debug_info)) {
                    internal_details.assign(debug_info.detail());
                } else if (detail.UnpackTo(&subject)) {
                    // do nothing
                }
            }
        }
    }

    struct AccessKeySignatureParametersConverter {
        priv::accessservice::v2::AccessKeySignature* signature;

        void operator()(const Version2Parameters& parameters) {
            auto signature_method = static_cast<priv::accessservice::v2::AccessKeySignature_Version2Parameters_SignatureMethod>(parameters.GetSignatureMethod());
            signature->mutable_v2_parameters()->set_signature_method(signature_method);
        }

        void operator()(const Version4Parameters& parameters) {
            auto mutable_parameters = signature->mutable_v4_parameters();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(parameters.SignedAt().time_since_epoch()).count();
            mutable_parameters->mutable_signed_at()->set_seconds(seconds);
            mutable_parameters->mutable_region()->assign(parameters.Region());
            mutable_parameters->mutable_service()->assign(parameters.Service());
        }
    };

    template <typename REQUEST>
    struct CredentialsConverter {
        REQUEST& request;

        void operator()(const IamToken& token) {
            request.mutable_iam_token()->assign(token.Value());
        }

        void operator()(const ApiKey& api_key) {
            request.mutable_api_key()->assign(api_key.Value());
        }

        void operator()(const AccessKeySignature& signature) {
            auto mutable_signature = request.mutable_signature();
            mutable_signature->mutable_access_key_id()->assign(signature.AccessKeyId());
            mutable_signature->mutable_string_to_sign()->assign(signature.SignedString());
            mutable_signature->mutable_signature()->assign(signature.Signature());
            std::visit(AccessKeySignatureParametersConverter{mutable_signature}, signature.Parameters());
        }
    };

    priv::accessservice::v2::AuthenticateRequest ToAuthenticateRequest(
        const Credentials& credentials) {
        priv::accessservice::v2::AuthenticateRequest result;
        std::visit(CredentialsConverter<priv::accessservice::v2::AuthenticateRequest>{result}, credentials);
        return result;
    }

    priv::accessservice::v2::AuthorizeRequest ToAuthorizeRequest(
        const Credentials& credentials,
        const std::string& permission,
        const std::vector<Resource>& resource_path) {
        priv::accessservice::v2::AuthorizeRequest result;
        std::visit(CredentialsConverter<priv::accessservice::v2::AuthorizeRequest>{result}, credentials);
        result.mutable_permission()->assign(permission);

        std::for_each(resource_path.begin(), resource_path.end(),
                      [&result](const Resource& resource) -> void {
                          auto proto_resource = result.add_resource_path();
                          proto_resource->mutable_type()->assign(resource.Type());
                          proto_resource->mutable_id()->assign(resource.Id());
                      });

        return result;
    }

    AuthenticationResult ToAuthenticationResult(
        AuthStatus status,
        const priv::accessservice::v2::AuthenticateResponse& response) {
        return {status, ToSubject(response.subject())};
    }

    AuthorizationResult ToAuthorizationResult(
        AuthStatus status,
        const priv::accessservice::v2::AuthorizeResponse& response) {
        return {status, ToSubject(response.subject())};
    }

}
