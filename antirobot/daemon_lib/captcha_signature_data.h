#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>


namespace NAntiRobot {


struct TCaptchaSignatureData {
    TString EncryptionKey;
    TString EncodedEncryptionKey;
    TString Signature;

    TCaptchaSignatureData() = default;

    explicit TCaptchaSignatureData(TStringBuf userAddr);

    TString EncryptEncode(TStringBuf cleartext) const;
};


} // namespace NAntiRobot
