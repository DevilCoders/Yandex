#include "captcha_check.h"
#include "eventlog_err.h"
#include "captcha_api_check.h"
#include "captcha_fury_check.h"
#include "environment.h"

#include <antirobot/lib/http_helpers.h>

namespace NAntiRobot {
    NThreading::TFuture<TCaptchaCheckResult> IsCaptchaGoodAsync(const TCaptchaKey& key,
                                                                const TRequestContext& rc,
                                                                const TAntirobotDaemonConfig& cfg,
                                                                TCaptchaSettingsPtr settings) {

        auto& env = rc.Env;
        TAtomicSharedPtr<const TRequest> request = rc.Req;

        if (key.Token.CaptchaType == ECaptchaType::SmartKeys) {
            TCaptchaCheckResult result;
            result.Success = false;
            result.PreprodSuccess = false;
            return NThreading::MakeFuture(std::move(result));
        }

        TCaptchaApiOptions captchaApiOptions = {
            cfg.CaptchaApiHost,
            cfg.CaptchaApiProtocol,
            cfg.CaptchaCheckTimeout,
        };
        TFuryOptions prodFuryOptions = {
            cfg.FuryEnabled && !env.DisablingFlags.IsStopFuryForAll(),
            cfg.FuryHost,
            cfg.FuryProtocol,
            cfg.FuryBaseTimeout,
            env.Tvm->GetServiceTicket(ANTIROBOT_DAEMON_CONFIG.FuryTVMClientId),
        };
        TFuryOptions preprodFuryOptions = {
            cfg.FuryPreprodEnabled && !env.DisablingFlags.IsStopFuryPreprodForAll(),
            cfg.FuryPreprodHost,
            cfg.FuryProtocol,
            cfg.FuryBaseTimeout,
            env.Tvm->GetServiceTicket(ANTIROBOT_DAEMON_CONFIG.FuryPreprodTVMClientId),
        };
        if (!prodFuryOptions.Enabled) {
            env.DisablingStat.AddStoppedFuryRequest();
        }
        if (!preprodFuryOptions.Enabled) {
            env.DisablingStat.AddStoppedFuryPreprodRequest();
        }

        if (key.Token.CaptchaType == ECaptchaType::SmartCheckbox) {
            // If Fury is disabled or the request matches a checkbox blacklist then the checkbox
            // will fail and the user will see a captcha.

            TFuture<TErrorOr<TCaptchaFuryResult>> prodFuture;
            TFuture<TErrorOr<TCaptchaFuryResult>> preprodFuture;

            if (rc.MatchedRules->CheckboxBlacklist) {
                prodFuture = NThreading::MakeFuture(TErrorOr(TCaptchaFuryResult{.CheckOk = false}));
                preprodFuture = prodFuture;
            } else {
                TCaptchaApiResult defaultCheckResult;
                defaultCheckResult.ImageCheckOk = false;

                prodFuture = GetCaptchaFuryResultAsync(
                    key, defaultCheckResult, *request,
                    prodFuryOptions,
                    settings,
                    env.CaptchaStat->GetFuryCheckResponseTimeStats()
                );

                preprodFuture = GetCaptchaFuryResultAsync(
                    key, defaultCheckResult, *request,
                    preprodFuryOptions,
                    settings,
                    env.CaptchaStat->GetFuryPreprodCheckResponseTimeStats()
                );
            }

            return NThreading::WaitAll(prodFuture.IgnoreResult(), preprodFuture.IgnoreResult()).Apply(
                [
                    prodFuture = std::move(prodFuture),
                    preprodFuture = std::move(preprodFuture),
                    request
                ] (const NThreading::TFuture<void>& waitResult) -> TCaptchaCheckResult {
                    waitResult.GetValue();
                    TCaptchaCheckResult result;

                    TCaptchaFuryResult captchaFuryPreprodResult;
                    if (TError err = preprodFuture.GetValue().PutValueTo(captchaFuryPreprodResult); err.Defined()) {
                        result.PreprodSuccess = false;
                        result.WasFuryPreprodError = true;
                        result.ErrorMessages.push_back(err->what());
                    } else {
                        result.PreprodSuccess = captchaFuryPreprodResult.CheckOk;
                        result.PreprodDegradation = captchaFuryPreprodResult.Degradation;
                    }

                    TCaptchaFuryResult captchaFuryResult;
                    if (TError err = prodFuture.GetValue().PutValueTo(captchaFuryResult); err.Defined()) {
                        // в случае ошибки капча считается не пройденной, тогда клиент перейдёт в режим картинки
                        result.Success = false;
                        result.WasFuryError = true;
                        result.ErrorMessages.push_back(err->what());
                    } else {
                        result.Success = captchaFuryResult.CheckOk;
                        result.Degradation = captchaFuryResult.Degradation;
                    }

                    if (request->CaptchaRequest.FailCheckbox) {
                        result.Success = result.PreprodSuccess = false;
                        result.WarningMessages.push_back("Forced fail by cookie");
                    }
                    if (request->UserAddr.IsYandex() && (request->Cookies.Has("YX_PASS_CHECKBOX") || request->CgiParams.Get("YX_PASS_CHECKBOX"))) {
                        result.Success = result.PreprodSuccess = true;
                        result.WarningMessages.push_back("Forced ok by cookie");
                    }

                    return result;
                }
            );
        }

        return GetCaptchaApiResultAsync(key, *rc.Req, captchaApiOptions, *rc.Env.CaptchaStat)
            .Apply([
                key,
                &env,
                request,
                prodFuryOptions,
                preprodFuryOptions,
                settings
            ] (const auto& future) -> NThreading::TFuture<TCaptchaCheckResult> {
                TCaptchaApiResult captchaApiResult;
                if (TError err = future.GetValue().PutValueTo(captchaApiResult); err.Defined()) {
                    TCaptchaCheckResult result;
                    result.WasApiCaptchaError = true;
                    result.ErrorMessages.push_back(err->what());
                    return NThreading::MakeFuture(result);
                }

                auto prodFuture = GetCaptchaFuryResultAsync(
                    key, captchaApiResult, *request,
                    prodFuryOptions,
                    settings,
                    env.CaptchaStat->GetFuryCheckResponseTimeStats()
                );
                auto preprodFuture = GetCaptchaFuryResultAsync(
                    key, captchaApiResult, *request,
                    preprodFuryOptions,
                    settings,
                    env.CaptchaStat->GetFuryPreprodCheckResponseTimeStats()
                );

                return NThreading::WaitAll(prodFuture.IgnoreResult(), preprodFuture.IgnoreResult()).Apply(
                    [
                        prodFuture = std::move(prodFuture),
                        preprodFuture = std::move(preprodFuture),
                        captchaApiResult
                    ] (const NThreading::TFuture<void>& waitResult) -> TCaptchaCheckResult {
                        waitResult.GetValue();
                        TCaptchaCheckResult result;
                        result.WarningMessages = captchaApiResult.Warnings;

                        TCaptchaFuryResult captchaFuryResult;
                        if (TError err = prodFuture.GetValue().PutValueTo(captchaFuryResult); err.Defined()) {
                            // в случае любой ошибки fallback на api-captcha
                            result.Success = captchaApiResult.ImageCheckOk;
                            result.WasFuryError = true;
                            result.ErrorMessages.push_back(err->what());
                        } else {
                            result.Success = captchaFuryResult.CheckOk;
                            result.Degradation = captchaFuryResult.Degradation;
                            result.FurySuccessChanged = captchaFuryResult.CheckOk != captchaApiResult.ImageCheckOk;
                        }

                        TCaptchaFuryResult captchaFuryPreprodResult;
                        if (TError err = preprodFuture.GetValue().PutValueTo(captchaFuryPreprodResult); err.Defined()) {
                            result.PreprodSuccess = captchaApiResult.ImageCheckOk;
                            result.WasFuryPreprodError = true;
                            result.ErrorMessages.push_back(err->what());
                        } else {
                            result.PreprodSuccess = captchaFuryPreprodResult.CheckOk;
                            result.PreprodDegradation = captchaFuryPreprodResult.Degradation;
                            result.FuryPreprodSuccessChanged = captchaFuryPreprodResult.CheckOk != captchaApiResult.ImageCheckOk;
                        }
                        return result;
                    }
                );
            })
            .Apply([request] (const auto& future) -> NThreading::TFuture<TCaptchaCheckResult> {
                TCaptchaCheckResult result = future.GetValue();
                if (request->UserAddr.IsYandex() && (request->Cookies.Has("YX_PASS_IMAGE") || request->CgiParams.Get("YX_PASS_IMAGE"))) {
                    result.Success = result.PreprodSuccess = true;
                    result.WarningMessages.push_back("Forced ok by cookie");
                }
                return NThreading::MakeFuture(result);
            });
    }

} // namespace NAntiRobot
