#include "antirobot_response.h"

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NAntiRobot {

/*
 * TResponse
 */

TResponse TResponse::ToUser(HttpCodes code, bool withExternalRequests) {
    return TResponse(code, true, withExternalRequests);
}

TResponse TResponse::ToBalancer(HttpCodes code, bool withExternalRequests) {
    return TResponse(code, false, withExternalRequests);
}

TResponse TResponse::Redirect(const TStringBuf& location, bool withExternalRequests) {
    static const TString LOCATION_HEADER = "Location";
    return ToUser(HTTP_FOUND, withExternalRequests).AddHeader(LOCATION_HEADER, location);
}

TResponse::TResponse(HttpCodes code, bool forwardToUser, bool withExternalRequests)
    : Resp(code)
    , WithExternalRequests(withExternalRequests)
    , ForwardToUser(forwardToUser)
{
}

IOutputStream& operator << (IOutputStream& os, const TResponse& response) {
    return os << response.Resp;
}

}
