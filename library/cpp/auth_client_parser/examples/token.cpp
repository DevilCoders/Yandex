#include <library/cpp/auth_client_parser/oauth_token.h>

#include <util/generic/strbuf.h>

namespace NExample {
    ui64 GetUidFromToken() {
        TString token = "AQAAAADue-GoAAAI2wSUpddcm0BIk-r70vL1O_A";
        NAuthClientParser::TOAuthToken oAuthToken;
        Y_ENSURE(oAuthToken.Parse(token));
        return oAuthToken.Uid();
    }
}
