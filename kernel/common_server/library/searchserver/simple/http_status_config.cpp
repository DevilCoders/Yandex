#include "http_status_config.h"

void THttpStatusManagerConfig::Init(const TYandexConfig::Directives& directives) {
#define Parse(x) x = directives.Value(#x, x)
    Parse(NotImplementedState);
    Parse(PaymentRequiredState);
    Parse(UnauthorizedStatus);
    Parse(TooManyRequestsStatus);
    Parse(IncompleteStatus);
    Parse(SyntaxErrorStatus);
    Parse(EmptyRequestStatus);
    Parse(EmptySetStatus);
    Parse(NoContentStatus);
    Parse(UnknownErrorStatus);
    Parse(TimeoutStatus);
    Parse(PartialStatus);
    Parse(NotFetchedStatus);
    Parse(NoShardsStatus);
    Parse(PartialThreshold);
    Parse(PermissionDeniedStatus);
    Parse(UserErrorState);
    Parse(ServiceUnavailable);
#undef Parse
}

void THttpStatusManagerConfig::ToString(IOutputStream& so) const {
#define ToString(x) so << #x << " : " << x << Endl;
    ToString(NotImplementedState);
    ToString(PaymentRequiredState);
    ToString(UnauthorizedStatus);
    ToString(TooManyRequestsStatus);
    ToString(IncompleteStatus);
    ToString(SyntaxErrorStatus);
    ToString(EmptyRequestStatus);
    ToString(EmptySetStatus);
    ToString(NoContentStatus);
    ToString(UnknownErrorStatus);
    ToString(TimeoutStatus);
    ToString(PartialStatus);
    ToString(NotFetchedStatus);
    ToString(NoShardsStatus);
    ToString(PartialThreshold);
    ToString(PermissionDeniedStatus);
    ToString(UserErrorState);
    ToString(ServiceUnavailable);
#undef ToString
}

THttpStatusManagerConfig::THttpStatusManagerConfig(const TYandexConfig::Directives& directives)
    : THttpStatusManagerConfig()
{
    Init(directives);
}
