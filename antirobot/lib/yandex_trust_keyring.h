#pragma once

#include "keyring_base.h"

#include <array>


namespace NAntiRobot {


struct TYandexTrustSecretKey {
    static constexpr size_t SignLen = 32;
    static constexpr size_t KeyLen = 32;

    explicit TYandexTrustSecretKey(const TString& hexStr);

    void Sign(TStringBuf data, unsigned char* out) const;

    TString Bytes;
};


class TYandexTrustKeyRing : public TKeyRingBase<TYandexTrustSecretKey, TYandexTrustKeyRing> {
    using TKeyRingBase::TKeyRingBase;
public:
    bool IsSignedHex(TStringBuf data, TStringBuf signature) const;
};


} // namespace NAntiRobot
