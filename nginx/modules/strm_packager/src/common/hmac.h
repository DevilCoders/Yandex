#pragma once

extern "C" {
#include <ngx_event_openssl.h>
}

#include <util/generic/string.h>

namespace NStrm::NPackager {
    namespace NHmac {
        typedef const TString THmacType;

        const THmacType Sha256 = "SHA256";
        const THmacType Md5 = "MD5";

        TString Hmac(THmacType& type, const TString& secret, const TString& message);
    }
}
