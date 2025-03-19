#pragma once

#include <util/generic/strbuf.h>

namespace NFastUserFactors {

    enum class ECTRType {
        CTR,
        CCTR,
        PCTR,
        CCTR2,
    };

    float GenericCTR(const float shows, const float clicks, const ECTRType type) noexcept;
    float CTR(const float shows, const float clicks) noexcept;
    float Pctr(const float shows, const float clicks) noexcept;
    float CCTR(const float shows, const float clicks) noexcept;
    float CCTR2(const float shows, const float clicks) noexcept;
    float Frc(const float clicks, const float queryClicks) noexcept;
    float RationalSigmoid(const float value, const float norm) noexcept;
    float WinLossesRatioWithPrior(const float wins, const float losses, const float prior) noexcept;

    ui8 FloatToChar(const float counter) noexcept;
    float CharToFloat(const ui8 byte) noexcept;

    ui32 UrlHash(const TStringBuf url);

}
