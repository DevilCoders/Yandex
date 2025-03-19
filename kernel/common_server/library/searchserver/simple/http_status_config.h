#pragma once

#include <library/cpp/yconf/conf.h>
#include <library/cpp/http/misc/httpcodes.h>

struct THttpStatusManagerConfig {
    int IncompleteStatus = HTTP_BAD_GATEWAY;
    int SyntaxErrorStatus = HTTP_BAD_REQUEST;
    int EmptyRequestStatus = HTTP_BAD_REQUEST;
    int EmptySetStatus = HTTP_NOT_FOUND;
    int NoContentStatus = HTTP_NO_CONTENT;
    int UnknownErrorStatus = HTTP_INTERNAL_SERVER_ERROR;
    int TimeoutStatus = HTTP_GATEWAY_TIME_OUT;
    int PartialStatus = HTTP_PARTIAL_CONTENT;
    int NotFetchedStatus = HTTP_OK;
    int NoShardsStatus = HTTP_NO_CONTENT;
    int TooManyRequestsStatus = HTTP_TOO_MANY_REQUESTS;
    int UnauthorizedStatus = HTTP_UNAUTHORIZED;
    int PermissionDeniedStatus = HTTP_FORBIDDEN;
    int ConflictRequest = HTTP_CONFLICT;
    int UserErrorState = HTTP_BAD_REQUEST;
    int PaymentRequiredState = HTTP_PAYMENT_REQUIRED;
    int NotImplementedState = HTTP_NOT_IMPLEMENTED;
    int HeadAppError = HTTP_GONE;
    float PartialThreshold = 0;
    int ServiceUnavailable = HTTP_SERVICE_UNAVAILABLE;
    int RequestSizeIsTooBig = HTTP_REQUEST_ENTITY_TOO_LARGE;

public:
    THttpStatusManagerConfig() = default;
    void Init(const TYandexConfig::Section* section) {
        Init(section->GetDirectives());
    }

    void Init(const TYandexConfig::Directives& directives);
    THttpStatusManagerConfig(const TYandexConfig::Directives& directives);

    void ToString(IOutputStream& so) const;
};
