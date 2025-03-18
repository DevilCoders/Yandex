#include <library/cpp/auth_client_parser/cookie.h>

#include <util/generic/strbuf.h>

namespace NExample {
    void ParseCookieZeroAllocation() {
        TString cookie = "3:1624446318.1.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.0M_i.abQ_s-kSWb0T646ff45";

        NAuthClientParser::TZeroAllocationCookie zeroAllocationCookie;
        zeroAllocationCookie.Parse(cookie);

        Y_ENSURE(zeroAllocationCookie.Status() == NAuthClientParser::EParseStatus::RegularMayBeValid);

        const NAuthClientParser::TSessionInfo& sessionInfo = zeroAllocationCookie.SessionInfo();
        Y_UNUSED(sessionInfo);

        const NAuthClientParser::TUserInfo& user = zeroAllocationCookie.User();
        Y_UNUSED(user);
    }

    void ParseCookieFull() {
        TString cookie = "3:1624446318.0.0.1234567890:ASDFGH:7f.101|111111.1.202.1:1234|m:YA_RU:23453.0M_i.abQ_s-kSWb0T646ff45";

        NAuthClientParser::TFullCookie fullCookie;
        fullCookie.Parse(cookie);

        Y_ENSURE(fullCookie.Status() == NAuthClientParser::EParseStatus::RegularMayBeValid);

        const NAuthClientParser::TSessionInfoExt& sessionInfo = fullCookie.SessionInfo();
        Y_UNUSED(sessionInfo);

        for (const NAuthClientParser::TUserInfoExt& user : fullCookie.Users()) {
            Y_UNUSED(user);
        }
    }
}
