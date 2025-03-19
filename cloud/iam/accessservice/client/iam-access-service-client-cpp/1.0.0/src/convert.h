#pragma once

#include "cloud-auth/subject.h"
#include "cloud-auth/client.h"

#include <yandex/cloud/priv/accessservice/v2/access_service.grpc.pb.h>
#include <list>

namespace yandex::cloud::auth::convert {

    AuthStatus ToStatus(grpc::StatusCode code);

    grpc::StatusCode ParseStatusCode(const std::string& str);

    std::vector<grpc::StatusCode> ParseStatusCodes(
        const std::vector<std::string>& codes,
        const std::vector<grpc::StatusCode>& defaultValue);

    Subject ToSubject(const priv::accessservice::v2::Subject& subject);

    void UnpackDetails(const grpc::Status& status, std::string& internal_details, yandex::cloud::priv::accessservice::v2::Subject& subject);

    template <typename RESULT>
    RESULT ToError(
        const grpc::Status& status) {
        std::string internal_details;
        yandex::cloud::priv::accessservice::v2::Subject subject;
        UnpackDetails(status, internal_details, subject);
        return RESULT{Error{status.error_code(), status.error_message(), internal_details}, ToSubject(subject)};
    }

    AuthorizationResult ToAuthorizationResult(
        AuthStatus status,
        const priv::accessservice::v2::AuthorizeResponse& response);

    AuthenticationResult ToAuthenticationResult(
        AuthStatus status,
        const priv::accessservice::v2::AuthenticateResponse& response);

    priv::accessservice::v2::AuthenticateRequest ToAuthenticateRequest(
        const Credentials& credentials);

    priv::accessservice::v2::AuthorizeRequest ToAuthorizeRequest(
        const Credentials& credentials,
        const std::string& permission,
        const std::vector<Resource>& resource_path);

}
