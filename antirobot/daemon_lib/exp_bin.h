#pragma once

#include <antirobot/lib/enum.h>

#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/system/types.h>

#include <array>


namespace NAntiRobot {


enum class EExpBin : ui8 {
    Count = 4,
};


} // namespace NAntiRobot


inline const TString& ToString(NAntiRobot::EExpBin expBin) {
    static const struct TStringValues {
        TStringValues() {
            for (size_t i = 0; i < EnumValue(NAntiRobot::EExpBin::Count); ++i) {
                Values[i] = ToString(i);
            }
        }

        std::array<TString, EnumValue(NAntiRobot::EExpBin::Count)> Values;
    } stringValues;

    Y_ENSURE(
        EnumValue(expBin) < stringValues.Values.size(),
        "invalid EExpBin passed to ToString"
    );

    return stringValues.Values[EnumValue(expBin)];
};
