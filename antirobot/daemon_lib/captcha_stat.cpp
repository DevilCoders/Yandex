#include "captcha_stat.h"
#include "environment.h"

namespace NAntiRobot {


void TCaptchaStat::UpdateOnInput(
    const TCaptchaKey& key,
    const TRequestContext& rc,
    const TCaptchaCheckResult& captchaCheckResult,
    const TCaptchaSettingsPtr& settings
) {
    const TRequest& req = *rc.Req;
    if (key.Token.CaptchaType == ECaptchaType::SmartKeys) {
        CaptchaApiServiceCounters.Inc(req, TCaptchaStat::ECaptchaApiServiceCounter::KeyLoads);
        return;
    }

    if (captchaCheckResult.Success) {
        if (key.Token.CaptchaType == ECaptchaType::SmartCheckbox) {
            ServiceNsExpBinCounters.Inc(req, TCaptchaStat::EServiceNsExpBinCounter::CorrectCheckboxInputs);
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::CheckboxSuccess, settings);
        } else {
            ServiceNsExpBinCounters.Inc(req, TCaptchaStat::EServiceNsExpBinCounter::CorrectAdvancedInputs);
            if (captchaCheckResult.FurySuccessChanged) {
                ServiceCounters.Inc(req, EServiceCounter::FuryFailToOkChanges);
            }
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::AdvancedSuccess, settings);
        }
    } else {
        if (key.Token.CaptchaType == ECaptchaType::SmartCheckbox) {
            ServiceCounters.Inc(req, EServiceCounter::IncorrectCheckboxInputs);
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::CheckboxFailed, settings);
        } else {
            if (req.TrustedUser) {
                ServiceCounters.Inc(req, EServiceCounter::IncorrectAdvancedInputsTrustedUsers);
            }
            ServiceCounters.Inc(req, EServiceCounter::IncorrectAdvancedInputs);
            if (captchaCheckResult.FurySuccessChanged) {
                ServiceCounters.Inc(req, EServiceCounter::FuryOkToFailChanges);
            }
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::AdvancedFailed, settings);
        }
    }

    if (captchaCheckResult.PreprodSuccess) {
        if (key.Token.CaptchaType == ECaptchaType::SmartCheckbox) {
            ServiceCounters.Inc(req, EServiceCounter::PreprodCorrectCheckboxInputs);
            if (req.TrustedUser) {
                ServiceCounters.Inc(req, EServiceCounter::PreprodCorrectCheckboxInputsTrustedUsers);
            }
        } else {
            ServiceCounters.Inc(req, EServiceCounter::PreprodCorrectAdvancedInputs);
            if (req.TrustedUser) {
                ServiceCounters.Inc(req, EServiceCounter::PreprodCorrectAdvancedInputsTrustedUsers);
            }
            if (captchaCheckResult.FuryPreprodSuccessChanged) {
                ServiceCounters.Inc(req, EServiceCounter::FuryPreprodFailToOkChanges);
            }
        }
    } else {
        if (key.Token.CaptchaType == ECaptchaType::SmartCheckbox) {
            ServiceCounters.Inc(req, EServiceCounter::PreprodIncorrectCheckboxInputs);
            if (req.TrustedUser) {
                ServiceCounters.Inc(req, EServiceCounter::PreprodIncorrectCheckboxInputsTrustedUsers);
            }
        } else {
            ServiceCounters.Inc(req, EServiceCounter::PreprodIncorrectAdvancedInputs);
            if (req.TrustedUser) {
                ServiceCounters.Inc(req, EServiceCounter::PreprodIncorrectAdvancedInputsTrustedUsers);
            }
            if (captchaCheckResult.FuryPreprodSuccessChanged) {
                ServiceCounters.Inc(req, EServiceCounter::FuryPreprodOkToFailChanges);
            }
        }
    }
}

} // namespace NAntiRobot
