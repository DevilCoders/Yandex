#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

// NOTE: constructors and factory functions here throw TBadArgumentException
//       in case of parse error.

namespace NUgc {
    class TUserId {
    public:
        TUserId(const TStringBuf& userId);
        TUserId(const TString& userId);
        TUserId(const char* userId);

        bool IsLoggedIn() const;
        const TString AsString() const;
        TString GetPassportUID() const;
        TString GetYandexUID() const;

        static TUserId FromPassportUID(const TString& puid);
        static TUserId FromPassportUID(ui64 puid);
        static TUserId FromYandexUID(const TString& yandexuid);
        static TUserId FromOrgId(const TString& oid);

    public:
        static const TVector<TString> ValidPrefixes;

    private:
        TString UserId; // User identifier for logged in users
    };

    inline bool operator==(const TUserId& l, const TUserId& r) {
        return l.AsString() == r.AsString();
    }

    inline bool operator!=(const TUserId& l, const TUserId& r) {
        return !operator==(l, r);
    }

} // namespace NUgc
