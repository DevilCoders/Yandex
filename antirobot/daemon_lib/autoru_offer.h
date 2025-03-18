#pragma once
#include <contrib/libs/re2/re2/re2.h>

namespace NAntiRobot {

class TAutoruOfferDetector {
public:
    explicit TAutoruOfferDetector(const TStringBuf& salt);

    bool Process(TStringBuf doc) const;
private:
    bool IsSigned(TStringBuf data, TStringBuf signature) const;
    bool IsSignedHex(TStringBuf data, TStringBuf signature) const;

    const TStringBuf Salt;
    static const re2::RE2 OfferPattern;
    static constexpr size_t SignLen = 16;
    static constexpr size_t SignHexLen = 8;
};

} // namespace NAntiRobot
