#include "factors.h"

#include <kernel/urlid/urlhash.h>

#include <util/generic/ymath.h>

#include <cmath>

namespace {

    float W(const float shows) noexcept {
        return (2.0f / (1.0f + std::exp(-shows))) - 1.0f;
    }

    float FRC(const float queryClicks) noexcept {
        return 1.0f / (1.0f + std::pow(2.0f, 10.0f - queryClicks));
    }

    float PCTR(const float shows, const float clicks) noexcept {
        return FRC(Min(shows, clicks));
    }

    // This means that compression method will make mistakes in case of counters
    // greater than exp(20). We are fine with that since float values about 1e7 are
    // not affected by += 1 operation.
    constexpr float MAX_LOG_VALUE = 20.0f;

}

namespace NFastUserFactors {

    float GenericCTR(const float shows, const float clicks, const ECTRType type) noexcept {
        switch (type) {
            case ECTRType::CTR:
                return CTR(shows, clicks);
            case ECTRType::CCTR:
                return CCTR(shows, clicks);
            case ECTRType::PCTR:
                return Pctr(shows, clicks);
            case ECTRType::CCTR2:
                return CCTR2(shows, clicks);
        }
    }

    float CTR(const float shows, const float clicks) noexcept {
        return std::abs(shows) > std::numeric_limits<float>::epsilon()
            ? clicks / shows
            : 0.0f;
    }

    float Pctr(const float shows, const float clicks) noexcept {
        if (FuzzyEquals(shows + 1.f, 1.f) || FuzzyEquals(clicks + 1.f, 1.f)) {
            return 0.0f;
        }

        return Min(clicks, shows) / shows * PCTR(shows, clicks);
    }

    float CCTR(const float shows, const float clicks) noexcept {
        return 0.5f * (1.0f - W(shows) + CTR(shows, clicks));
    }

    float CCTR2(const float shows, const float clicks) noexcept {
        return 0.5 * (1 - W(shows)) + CTR(shows, clicks) * W(shows);
    }

    float Frc(const float clicks, const float queryClicks) noexcept {
        if (FuzzyEquals(queryClicks + 1.f, 1.f) || FuzzyEquals(clicks + 1.f, 1.f)) {
            return 0.0f;
        }

        return clicks / queryClicks * FRC(queryClicks);
    }

    float RationalSigmoid(const float value, const float norm) noexcept {
        return value / (value + norm);
    }

    float WinLossesRatioWithPrior(const float wins, const float losses, const float prior) noexcept {
        return (wins + 2.0f * prior) / (wins + losses + 2.0f);
    }

    ui8 FloatToChar(const float counter) noexcept {
        return 255.0f * Min(std::log(1.0f + counter) / MAX_LOG_VALUE, 1.0f);
    }

    float CharToFloat(const ui8 byte) noexcept {
        // Note that here we loose some information on byte 0: only 0 counters are
        // mapped to 0. It is done intentionally since it is better to keep
        // old good default values 0, 0.5 and 1 wher we have no data.
        return std::exp(byte * MAX_LOG_VALUE / 255.0f) - 1.0f;
    }

    ui32 UrlHash(const TStringBuf url) {
        return static_cast<ui32>(UrlHash64(url) >> 32L);
    }

}
