#pragma once

#include "unified_agent_log.h"
#include "cloud_grpc_client.h"


namespace NAntiRobot {

enum class ECaptchaUserMetricKey {
    CheckboxShows             /* "smartcaptcha.captcha.checkbox.shows" */,
    CheckboxSuccess           /* "smartcaptcha.captcha.checkbox.success_count" */,
    CheckboxFailed            /* "smartcaptcha.captcha.checkbox.failed_count" */,
    AdvancedShows             /* "smartcaptcha.captcha.advanced.shows" */,
    AdvancedSuccess           /* "smartcaptcha.captcha.advanced.success_count" */,
    AdvancedFailed            /* "smartcaptcha.captcha.advanced.failed_count" */,
    ValidateSuccess           /* "smartcaptcha.captcha.validate.success_count" */,
    ValidateFailed            /* "smartcaptcha.captcha.validate.failed_count" */,
    Count
};

class TUnifiedAgentBillingLog : public TLog {
public:
    explicit TUnifiedAgentBillingLog(const TString& unifiedAgentUri);
};

class TUnifiedAgentResourceMetrics : public TLog {
public:
    explicit TUnifiedAgentResourceMetrics(const TString& unifiedAgentUri);
    void Track(ECaptchaUserMetricKey key, const TCaptchaSettingsPtr& settings);
};

} // namespace NAntiRobot
