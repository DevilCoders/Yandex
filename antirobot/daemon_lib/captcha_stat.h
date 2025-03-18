#pragma once

#include "captcha_check.h"
#include "captcha_key.h"
#include "exp_bin.h"
#include "request_params.h"
#include "stat.h"
#include "time_stats.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/lib/stats_writer.h>

#include <util/system/defaults.h>

#include <atomic>


namespace NAntiRobot {

class TCaptchaStat {
public:
    enum class ECounter {
        RequestsAll              /* "captcha_requests_all" */,
        RequestsFrom1stTry       /* "captcha_requests_1st_try" */,
        RequestsFrom2ndTry       /* "captcha_requests_2nd_try" */,
        RequestsFrom3rdTry       /* "captcha_requests_3rd_try" */,
        WebMainRedirects         /* "captcha_web_main_redirects" */,
        CaptchaGenerationErrors  /* "captcha_generation_errors" */,
        Count
    };

    TCategorizedStats<std::atomic<size_t>, ECounter> Counters;

    enum class EServiceCounter {
        FuryFailToOkChanges                         /* "captcha_fury_fail_to_ok_changes" */,
        FuryOkToFailChanges                         /* "captcha_fury_ok_to_fail_changes" */,
        FuryPreprodFailToOkChanges                  /* "captcha_fury_preprod_fail_to_ok_changes" */,
        FuryPreprodOkToFailChanges                  /* "captcha_fury_preprod_ok_to_fail_changes" */,
        IncorrectAdvancedInputs                     /* "captcha_incorrect_inputs" */,
        IncorrectAdvancedInputsTrustedUsers         /* "captcha_incorrect_inputs_trusted_users" */,
        IncorrectCheckboxInputs                     /* "captcha_incorrect_checkbox_inputs" */,
        IncorrectCheckboxInputsTrustedUsers         /* "captcha_incorrect_checkbox_inputs_trusted_users" */,
        PreprodCorrectAdvancedInputs                /* "captcha_preprod_correct_inputs" */,
        PreprodCorrectAdvancedInputsTrustedUsers    /* "captcha_preprod_correct_inputs_trusted_users" */,
        PreprodCorrectCheckboxInputs                /* "captcha_preprod_correct_checkbox_inputs" */,
        PreprodCorrectCheckboxInputsTrustedUsers    /* "captcha_preprod_correct_checkbox_inputs_trusted_users" */,
        PreprodIncorrectAdvancedInputs              /* "captcha_preprod_incorrect_inputs" */,
        PreprodIncorrectAdvancedInputsTrustedUsers  /* "captcha_preprod_incorrect_inputs_trusted_users" */,
        PreprodIncorrectCheckboxInputs              /* "captcha_preprod_incorrect_checkbox_inputs" */,
        PreprodIncorrectCheckboxInputsTrustedUsers  /* "captcha_preprod_incorrect_checkbox_inputs_trusted_users" */,
        VoiceDownloads                              /* "captcha_voice_downloads" */,
        Count
    };

    enum class ECaptchaApiServiceCounter {
        KeyLoads                                    /* "captcha_key_loads" */,

        ValidateSuccess                             /* "captcha_validate_success" */,
        ValidateFailed                              /* "captcha_validate_failed" */,
        ValidateRequests                            /* "captcha_validate_requests" */,
        ValidateApiFailedRequests                   /* "captcha_validate_api_failed_requests" */,
        ValidateInvalidSecretRequests               /* "captcha_validate_invalid_secret_requests" */,
        ValidateValidSecretRequests                 /* "captcha_validate_valid_secret_requests" */,
        ValidateUnauthenticatedRequests             /* "captcha_validate_unauthenticated_requests" */,
        ValidateInvalidSpravkaRequests              /* "captcha_validate_invalid_spravka_requests" */,
        ValidateUnknownErrors                       /* "captcha_validate_unknown_errors" */,
        ValidateSuspended                           /* "captcha_validate_suspended_requests" */,

        ValidateTvmSuccessRequests                  /* "captcha_validate_tvm_success_requests" */,
        ValidateTvmDeniedRequests                   /* "captcha_validate_tvm_denied_requests" */,
        ValidateTvmUnknownErrors                    /* "captcha_validate_tvm_unknown_errors" */,

        IframeAllowedRequests                       /* "captcha_iframe_allowed_requests" */,
        IframeWrongDomainRequests                   /* "captcha_iframe_wrong_domain_requests" */,
        IframeInvalidSitekeyRequests                /* "captcha_iframe_invalid_sitekey_requests" */,
        IframeApiFailedRequests                     /* "captcha_iframe_api_failed_requests" */,
        IframeCheckboxShows                         /* "captcha_iframe_checkbox_shows" */,
        IframeAdvancedShows                         /* "captcha_iframe_advanced_shows" */,
        IframeSuspendedRequests                     /* "captcha_iframe_suspended_requests" */,

        CheckApiFailedRequests                      /* "captcha_check_api_failed_requests" */,
        CheckInvalidSitekeyRequests                 /* "captcha_check_invalid_sitekey_requests" */,
        CheckValidSitekeyRequests                   /* "captcha_check_valid_sitekey_requests" */,
        CheckUnauthorizedRequests                   /* "captcha_check_unauthorized_requests" */,

        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceCounter,
        EHostType
    > ServiceCounters;

    TCategorizedStats<
        std::atomic<size_t>, ECaptchaApiServiceCounter
    > CaptchaApiServiceCounters;

    enum class EServiceNsCounter {
        AdvancedShows                      /* "captcha_advanced_shows" */,
        CheckboxShows                      /* "captcha_shows" */,
        CorrectAdvancedInputsTrustedUsers  /* "captcha_correct_inputs_trusted_users" */,
        CorrectCheckboxInputsTrustedUsers  /* "captcha_correct_checkbox_inputs_trusted_users" */,
        InitialRedirects                   /* "captcha_initial_redirects" */,
        RedirectsByCbb                     /* "captcha_redirects_by_cbb" */,
        RedirectsByMxnet                   /* "captcha_redirects_by_mxnet" */,
        RedirectsByYql                     /* "captcha_redirects_by_yql" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceNsCounter,
        EHostType, ESimpleUidType
    > ServiceNsCounters;

    enum class EServiceNsExpBinCounter {
        CorrectAdvancedInputs              /* "captcha_correct_inputs" */,
        CorrectCheckboxInputs              /* "captcha_correct_checkbox_inputs" */,
        ImageShows                         /* "captcha_image_shows" */,
        CheckboxRedirects                  /* "captcha_redirects_bin" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceNsExpBinCounter,
        EHostType, ESimpleUidType, EExpBin
    > ServiceNsExpBinCounters;

    enum class EServiceNsGroupCounter {
        AdvancedRedirects              /* "captcha_advanced_redirects" */,
        AdvancedRedirectsTrustedUsers  /* "captcha_advanced_redirects_trusted_users" */,
        CheckboxRedirectsTrustedUsers  /* "captcha_redirects_trusted_users" */,
        CheckboxRedirects              /* "captcha_redirects" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceNsGroupCounter,
        EHostType, ESimpleUidType, EReqGroup
    > ServiceNsGroupCounters;

public:
    explicit TCaptchaStat(std::array<size_t, HOST_NUMTYPES> groupCounts)
        : ServiceNsGroupCounters(groupCounts)
        , ApiCaptchaGenerateResponseTime(TIME_STATS_VEC_CACHER, "captcha_response_")
        , ApiCaptchaCheckResponseTime(TIME_STATS_VEC_LARGE, "api_captcha_check_response_")
        , FuryCheckResponseTime(TIME_STATS_VEC_LARGE, "fury_check_response_")
        , FuryPreprodCheckResponseTime(TIME_STATS_VEC_LARGE, "fury_preprod_check_response_")
    {}

    void IncErrorCount() {
        Counters.Inc(ECounter::CaptchaGenerationErrors);
    }

    void IncRequestCount() {
        Counters.Inc(ECounter::RequestsAll);
    }

    void UpdateTriesStat(int attempt) {
        switch (attempt) {
        case 0:
            Counters.Inc(ECounter::RequestsFrom1stTry);
            break;
        case 1:
            Counters.Inc(ECounter::RequestsFrom2ndTry);
            break;
        case 2:
            Counters.Inc(ECounter::RequestsFrom3rdTry);
            break;
        default:
            break;
        }
    }

    void UpdateOnRedirect(
        const TRequest& req,
        const TCaptchaToken& token,
        bool again,
        const NAntirobotEvClass::TBanReasons& banReasons
    ) {
        if (token.CaptchaType == ECaptchaType::SmartKeys) {
            return;
        }

        if (token.CaptchaType == ECaptchaType::SmartCheckbox) {
            if (req.TrustedUser) {
                ServiceNsGroupCounters.Inc(req, TCaptchaStat::EServiceNsGroupCounter::CheckboxRedirectsTrustedUsers);
            }
            ServiceNsGroupCounters.Inc(req, TCaptchaStat::EServiceNsGroupCounter::CheckboxRedirects);
            ServiceNsExpBinCounters.Inc(req, TCaptchaStat::EServiceNsExpBinCounter::CheckboxRedirects);
        } else {
            if (req.TrustedUser) {
                ServiceNsGroupCounters.Inc(req, TCaptchaStat::EServiceNsGroupCounter::AdvancedRedirectsTrustedUsers);
            }
            ServiceNsGroupCounters.Inc(req, TCaptchaStat::EServiceNsGroupCounter::AdvancedRedirects);
        }

        if (!again) {
            ServiceNsCounters.Inc(req, TCaptchaStat::EServiceNsCounter::InitialRedirects);
        }

        if (banReasons.GetMatrixnet()) {
            ServiceNsCounters.Inc(req, TCaptchaStat::EServiceNsCounter::RedirectsByMxnet);
        }

        if (banReasons.GetYql()) {
            ServiceNsCounters.Inc(req, TCaptchaStat::EServiceNsCounter::RedirectsByYql);
        }

        if (banReasons.GetCbb()) {
            ServiceNsCounters.Inc(req, TCaptchaStat::EServiceNsCounter::RedirectsByCbb);
        }

        if (req.HostType == HOST_WEB && req.ReqType == REQ_MAIN) {
            Counters.Inc(TCaptchaStat::ECounter::WebMainRedirects);
        }
    }

    void UpdateOnInput(
        const TCaptchaKey& key,
        const TRequestContext& rc,
        const TCaptchaCheckResult& captchaCheckResult,
        const TCaptchaSettingsPtr& settings
    );

    TTimeStats& GetApiCaptchaGenerateResponseTimeStats() {
        return ApiCaptchaGenerateResponseTime;
    }

    TTimeStats& GetApiCaptchaCheckResponseTimeStats() {
        return ApiCaptchaCheckResponseTime;
    }

    TTimeStats& GetFuryCheckResponseTimeStats() {
        return FuryCheckResponseTime;
    }

    TTimeStats& GetFuryPreprodCheckResponseTimeStats() {
        return FuryPreprodCheckResponseTime;
    }

    void Print(const TRequestGroupClassifier& groupClassifier, TStatsWriter& out) const {
        ApiCaptchaGenerateResponseTime.PrintStats(out);
        ApiCaptchaCheckResponseTime.PrintStats(out);
        FuryCheckResponseTime.PrintStats(out);
        FuryPreprodCheckResponseTime.PrintStats(out);

        Counters.Print(out);
        ServiceCounters.Print(out);
        ServiceNsCounters.Print(out);
        ServiceNsExpBinCounters.Print(out);
        ServiceNsGroupCounters.Print(groupClassifier, out);

        if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
            CaptchaApiServiceCounters.Print(out);
        }
    }

private:
    TTimeStats ApiCaptchaGenerateResponseTime;
    TTimeStats ApiCaptchaCheckResponseTime;
    TTimeStats FuryCheckResponseTime;
    TTimeStats FuryPreprodCheckResponseTime;
};


} // namespace NAntiRobot
