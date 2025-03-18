#include "captcha_freq_checker.h"

#include "config_global.h"

namespace NAntiRobot {

    bool TCaptchaInputFrequencyChecker::CheckForBan(ui64 nowMicroseconds, bool& wasBanned) {
        TInstant now = TInstant::MicroSeconds(nowMicroseconds);
        wasBanned = Banned;

        if (LastCheckPoint == TInstant()) {
            LastCheckPoint = now;
            Counter = 1;
            Banned = false;
            return false;
        }

        if (Banned) {
            if (LastCheckPoint + ANTIROBOT_DAEMON_CONFIG.CaptchaFreqBanTime < now) {
                Banned = false;
                LastCheckPoint = now;
                Counter = 1;
                return false;
            }
            return true;
        }

        ++Counter;

        if (LastCheckPoint + ANTIROBOT_DAEMON_CONFIG.CaptchaFreqMaxInterval > now) {
            if (Counter > ANTIROBOT_DAEMON_CONFIG.CaptchaFreqMaxInputs) {
                Banned = true;
                LastCheckPoint = now;
                return true;
            }
            return false;
        }

        Counter = 1;
        LastCheckPoint = now;

        return false;
    }
}
