#pragma once

#include <util/datetime/base.h>
#include <util/generic/typetraits.h>

namespace NAntiRobot {

    class TCaptchaInputFrequencyChecker {
    public:
        TCaptchaInputFrequencyChecker()
            : LastCheckPoint()
            , Counter(0)
            , Banned(false)
        {
        }

        bool CheckForBan(ui64 nowMicroseconds, bool& wasBanned);

        void Reset() {
            LastCheckPoint = TInstant();
            Counter = 0;
            Banned = false;
        }
    private:
        TInstant LastCheckPoint;
        ui32 Counter;
        bool Banned;
    };
}

Y_DECLARE_PODTYPE(NAntiRobot::TCaptchaInputFrequencyChecker);
