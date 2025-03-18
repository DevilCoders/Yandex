#pragma once

#include <antirobot/lib/antirobot_response.h>
#include <antirobot/lib/error.h>
#include <antirobot/daemon_lib/captcha_key.h>
#include <antirobot/daemon_lib/request_params.h>
#include <antirobot/daemon_lib/captcha_check.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

#include <functional>
#include <utility>

class IOutputStream;

namespace NAntiRobot {

struct TRequestContext;

extern const TResponse NOT_A_ROBOT;

NThreading::TFuture<TResponse> HandleShowCaptcha(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleCaptchaPgrd(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleShowCaptchaImage(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleCheckCaptchaApi(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleCheckCaptcha(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleCheckCaptchaCommon(TRequestContext& rc, TCaptchaSettingsPtr settings);
NThreading::TFuture<TResponse> HandleGeneralRequest(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleXmlSearch(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleVoice(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleVoiceIntro(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleCaptchaIframeHtml(TRequestContext& rc);
NThreading::TFuture<TResponse> TestCaptchaPage(TRequestContext& rc);
NThreading::TFuture<TResponse> TestBlockedPage(TRequestContext& rc);
NThreading::TFuture<TResponse> TestManyRequestsPage(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleProcessRequest(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleProcessCaptchaInput(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleMessengerRequest(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleCaptchaValidate(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleCaptchaDemo(TRequestContext& rc);
NThreading::TFuture<TResponse> RedirectToCaptcha(const TRequestContext& rc, bool again, bool fromCaptcha,
                                                 const TCaptchaToken& token, bool withExternalRequests = false,
                                                 TCaptchaSettingsPtr settings = TCaptchaSettingsPtr{});

bool CutRequestForwarding(TRequestContext& rc);
bool NotBanRequest(TRequestContext& rc);

} // namespace NAntiRobot
