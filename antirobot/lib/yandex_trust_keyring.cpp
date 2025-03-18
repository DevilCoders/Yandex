#include "yandex_trust_keyring.h"

#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/hmac.h>
#include <contrib/libs/openssl/include/openssl/sha.h>

#include <util/generic/yexception.h>
#include <util/string/hex.h>

#include <array>


namespace NAntiRobot {

namespace {
    void Hmac(TStringBuf key, TStringBuf data, unsigned char* out) {
        const auto* evp = EVP_sha256();
        ui32 hl = SHA256_DIGEST_LENGTH;
        Y_ENSURE(evp);
        const auto* res = HMAC(evp, key.data(), key.size(), reinterpret_cast<const unsigned char*>(data.data()), data.size(), out, &hl);
        Y_ENSURE(res == out);
        Y_ENSURE(hl == SHA256_DIGEST_LENGTH);
    }
}

TYandexTrustSecretKey::TYandexTrustSecretKey(const TString& hexStr) {
    Y_ENSURE(hexStr.size() == KeyLen * 2);
    Bytes = HexDecode(hexStr.data(), hexStr.size());
}

void TYandexTrustSecretKey::Sign(TStringBuf data, unsigned char* out) const {
    return Hmac(TStringBuf(Bytes), data, out);
}

bool TYandexTrustKeyRing::IsSignedHex(TStringBuf data, TStringBuf signature) const {
    static_assert(SHA256_DIGEST_LENGTH == TYandexTrustSecretKey::SignLen);
    unsigned char sign[TYandexTrustSecretKey::SignLen];
    char signHex[TYandexTrustSecretKey::SignLen * 2];
    TStringBuf signHexBuf(signHex, TYandexTrustSecretKey::SignLen * 2);

    // Look through all the keys, to be able to check data signed with outdated key
    for (const auto& key : Keys) {
        key.Sign(data, sign);
        HexEncode(sign, TYandexTrustSecretKey::SignLen, signHex);

        if (signHexBuf == signature) {
            return true;
        }
    }

    return false;
}

} // namespace NAntiRobot
