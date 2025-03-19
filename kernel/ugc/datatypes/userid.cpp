#include "userid.h"

#include <util/generic/yexception.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

// Emulating text from Y_ENSURE which was previously used here. Some tests depend on this
// particular message.
#define ARGUMENT_VALIDITY_ASSERT(condition) Y_ENSURE_EX((condition), TBadArgumentException() \
    << "Condition violated: `" #condition "'")

#define ARGUMENT_VALIDITY_ASSERT_EX(condition, message) Y_ENSURE_EX((condition), \
    TBadArgumentException() << (message))

template<>
NUgc::TUserId FromStringImpl<NUgc::TUserId>(const char* s, size_t len) {
    return NUgc::TUserId(TString(s, len));
}

namespace NUgc {
    const TVector<TString> TUserId::ValidPrefixes = {
        "/user/",
        "/visitor/",
        "/mobapp/",
        "/orguser/",
        "/partner_user/",
    };

    static bool StartsWithValidPrefix(const TString& userId) {
        for (const TString& prefix : TUserId::ValidPrefixes) {
            if (userId.StartsWith(prefix)) {
                return true;
            }
        }
        return false;
    }

    static void ValidateSymbols(const TString& userId) {
        size_t slashCount = 1; // Including initial slash.
        for (size_t i = 1; i < userId.size(); ++i) {
            if (userId[i] == '/') {
                ++slashCount;
                ARGUMENT_VALIDITY_ASSERT_EX(userId[i - 1] != '/', "Two slashes together");
            }
        }
        ARGUMENT_VALIDITY_ASSERT_EX(slashCount >= 2, "Slashes count must be >= 2");
    }

    TUserId::TUserId(const char* userId)
        : TUserId(TString(userId))
    {
    }

    TUserId::TUserId(const TStringBuf& userId)
        : TUserId(TString(userId))
    {
    }

    TUserId::TUserId(const TString& userId)
        : UserId(userId)
    {
        ARGUMENT_VALIDITY_ASSERT(!UserId.empty());
        ARGUMENT_VALIDITY_ASSERT(UserId[0] == '/');
        ARGUMENT_VALIDITY_ASSERT(UserId.back() != '/');
        ARGUMENT_VALIDITY_ASSERT(StartsWithValidPrefix(UserId));
        ValidateSymbols(UserId);
    }

    bool TUserId::IsLoggedIn() const {
        return UserId.StartsWith("/user/");
    }

    const TString TUserId::AsString() const {
        return UserId;
    }

    TString TUserId::GetPassportUID() const {
        TStringBuf s(UserId);

        if (!s.SkipPrefix("/user/")) {
            return TString();
        }

        for (size_t i = 0; i < s.size(); ++i) {
            if (!IsAsciiDigit(s[i])) {
                return TString();
            }
        }

        return TString(s);
    }

    TString TUserId::GetYandexUID() const {
        TStringBuf s(UserId);

        if (!s.SkipPrefix("/visitor/")) {
            return TString();
        }

        return TString(s);
    }

    TUserId TUserId::FromPassportUID(const TString& puid) {
        ui64 value;
        ARGUMENT_VALIDITY_ASSERT_EX(TryFromString(puid, value), "invalid passport id " + puid.Quote());
        return FromPassportUID(value);
    }

    TUserId TUserId::FromPassportUID(ui64 puid) {
        return TUserId(TStringBuilder() << "/user/" << puid);
    }

    TUserId TUserId::FromYandexUID(const TString& yandexuid) {
        return TUserId(TStringBuilder() << "/visitor/" << yandexuid);
    }

    TUserId TUserId::FromOrgId(const TString& oid) {
        ui64 value;
        ARGUMENT_VALIDITY_ASSERT_EX(TryFromString(oid, value), "invalid org id");
        return TUserId(TStringBuilder() << "/orguser/" << oid);
    }

} // namespace NUgc
