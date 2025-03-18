#include "captcha_signature_data.h"

#include <antirobot/lib/keyring.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/datetime/base.h>
#include <util/random/random.h>
#include <util/string/builder.h>


namespace NAntiRobot {


TCaptchaSignatureData::TCaptchaSignatureData(TStringBuf userAddr) {
    for (size_t i = 0; i < 4; ++i) {
        // We don't really need crypto randomness because it's just obfuscation.
        const auto chunk = RandomNumber<ui64>();
        const TStringBuf chunkBytes(reinterpret_cast<const char*>(&chunk), sizeof(chunk));
        EncryptionKey += chunkBytes;
    }

    EncodedEncryptionKey = Base64Encode(EncryptionKey);

    const auto timestamp = TInstant::Now().Seconds();
    const auto rnd = RandomNumber<ui64>();
    const auto signatureInput = TStringBuilder()
        << EncodedEncryptionKey << '_'
        << "1_"
        << timestamp << '_'
        << userAddr << '_'
        << rnd;

    const auto signature = TKeyRing::Instance()->SignHex(signatureInput);

    Signature = TStringBuilder()
        << "1_"
        << timestamp << '_'
        << rnd << '_'
        << signature;
}

TString TCaptchaSignatureData::EncryptEncode(TStringBuf cleartext) const {
    TString ret;
    ret.reserve(cleartext.size());

    for (size_t i = 0; i < cleartext.size(); ++i) {
        ret.push_back(cleartext[i] ^ EncryptionKey[i % EncryptionKey.size()]);
    }

    return Base64Encode(ret);
}


} // namespace NAntiRobot
