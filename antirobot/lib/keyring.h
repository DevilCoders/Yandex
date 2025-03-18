#pragma once

#include "keyring_base.h"


namespace NAntiRobot {


struct TSecretKey {
    static constexpr size_t SignLen = 16;
    static constexpr size_t SignHexLen = 32;

    explicit TSecretKey(TString bytes) : Bytes(std::move(bytes)) {}

    void Sign(TStringBuf data, char* out) const;
    void SignHex(TStringBuf data, char* out) const;

    TString Bytes;
};


class TKeyRing : public TKeyRingBase<TSecretKey, TKeyRing> {
    using TKeyRingBase::TKeyRingBase;
public:
    bool IsSigned(TStringBuf data, TStringBuf signature) const;
    bool IsSignedHex(TStringBuf data, TStringBuf signature) const;
    void Sign(TStringBuf data, char* tmp) const;
    TString SignHex(TStringBuf data) const;
private:
    const TSecretKey& Get() const;
};


} // namespace NAntiRobot
