#pragma once

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NLCookie {
    struct TLCookie {
        ui32 Timestamp = 0;
        ui64 Uid = 0;
        TString Login;
    };

    struct IKeychain {
        virtual ~IKeychain(){};

        // Returns empty string if the key was not found.
        virtual TStringBuf GetKey(const TStringBuf& keyId) const = 0;
    };

    // In case of error, returns false.
    // Since IKeychain::GetKey returns TStringBuf, the caller of this method
    // must guarantee, that keychain (or at least the selected key) is not
    // changed until this function returns. For example see the unit tests.
    bool TryParse(TLCookie& cookie, const TStringBuf& l, const IKeychain& keychain, bool checkSign = true);

    inline TMaybe<TLCookie> TryParse(const TStringBuf& l, const IKeychain& keychain, bool checkSign = true) {
        TLCookie cookie;
        if (TryParse(cookie, l, keychain, checkSign)) {
            return cookie;
        }
        return Nothing();
    }

}
