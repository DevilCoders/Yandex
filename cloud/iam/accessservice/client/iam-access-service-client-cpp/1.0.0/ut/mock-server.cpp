#include "../src/convert.h"

#include <google/rpc/error_details.pb.h>
#include <google/rpc/status.pb.h>

#include <grpcpp/grpcpp.h>
#include <regex>

static const std::regex SEMICOLON(":");

static std::vector<std::string> Split(const std::string& str, const std::regex& delimiter) {
    return {std::sregex_token_iterator(str.begin(), str.end(), delimiter, -1), std::sregex_token_iterator{}};
}

static void FillSubject(const std::vector<std::string>& params, yandex::cloud::priv::accessservice::v2::Subject* subject) {
    auto subject_type = params[1];
    if (subject_type == "SA") {
        auto serviceAccount = subject->mutable_service_account();
        serviceAccount->mutable_id()->assign(params[2]);
        serviceAccount->mutable_folder_id()->assign(params[3]);
    } else if (subject_type == "UA") {
        auto userAccount = subject->mutable_user_account();
        userAccount->mutable_id()->assign(params[2]);
        userAccount->mutable_federation_id()->assign(params[3]);
    } else {
        subject->mutable_anonymous_account();
    }
}

static std::string GetParamsString(const ::yandex::cloud::priv::accessservice::v2::AuthenticateRequest* request) {
    std::string value;
    switch (request->credentials_case()) {
        case yandex::cloud::priv::accessservice::v2::AuthenticateRequest::kIamToken:
            value = request->iam_token();
            break;
        case yandex::cloud::priv::accessservice::v2::AuthenticateRequest::kApiKey:
            value = request->api_key();
            break;
        case yandex::cloud::priv::accessservice::v2::AuthenticateRequest::kSignature:
            value = request->signature().access_key_id();
            break;
        default:
            break;
    }

    return value;
}

static std::string GetParamsString(const ::yandex::cloud::priv::accessservice::v2::AuthorizeRequest* request) {
    std::string value;
    switch (request->identity_case()) {
        case yandex::cloud::priv::accessservice::v2::AuthorizeRequest::kIamToken:
            value = request->iam_token();
            break;
        case yandex::cloud::priv::accessservice::v2::AuthorizeRequest::kApiKey:
            value = request->api_key();
            break;
        case yandex::cloud::priv::accessservice::v2::AuthorizeRequest::kSignature:
            value = request->signature().access_key_id();
            break;
        default:
            break;
    }

    return value;
}

static grpc::Status InternalAuthenticate(const std::string& str, yandex::cloud::priv::accessservice::v2::Subject* subject) {
    auto params = Split(str, SEMICOLON);
    auto status_code = yandex::cloud::auth::convert::ParseStatusCode(params[0]);
    if (status_code == grpc::StatusCode::DO_NOT_USE) {
        return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "BAD status code",
#ifdef ARCADIA_BUILD
          TString(params[0])
#else
          params[0]
#endif
        };
    }
    if (status_code == grpc::StatusCode::OK) {
        FillSubject(params, subject);
        return grpc::Status::OK;
    } else if (status_code == grpc::StatusCode::PERMISSION_DENIED || status_code == grpc::StatusCode::UNAUTHENTICATED) {
        google::rpc::Status details;
        FillSubject(params, subject);
        details.mutable_details()->Add()->PackFrom(*subject);
        return grpc::Status{status_code, "", details.SerializeAsString()};
    } else {
        google::rpc::Status details;
        google::rpc::DebugInfo debug_info;
        debug_info.mutable_detail()->assign(params[2]);
        details.mutable_details()->Add()->PackFrom(debug_info);
        return grpc::Status{status_code,
#ifdef ARCADIA_BUILD
            TString(params[1]),
#else
            params[1],
#endif
        details.SerializeAsString()};
    }
}

class MockAccessService: public yandex::cloud::priv::accessservice::v2::AccessService::Service {
public:
    grpc::Status Authenticate(::grpc::ServerContext*,
                              const ::yandex::cloud::priv::accessservice::v2::AuthenticateRequest* request,
                              ::yandex::cloud::priv::accessservice::v2::AuthenticateResponse* response) override {
        auto params = GetParamsString(request);
        return InternalAuthenticate(params, response->mutable_subject());
    }

    grpc::Status Authorize(::grpc::ServerContext*,
                           const ::yandex::cloud::priv::accessservice::v2::AuthorizeRequest* request,
                           ::yandex::cloud::priv::accessservice::v2::AuthorizeResponse* response) override {
        auto params = GetParamsString(request);
        auto status = InternalAuthenticate(params, response->mutable_subject());
        return status;
    }
};

std::shared_ptr<grpc::Server> CreateMockServer() {
    return grpc::ServerBuilder().RegisterService(new MockAccessService()).BuildAndStart();
}
