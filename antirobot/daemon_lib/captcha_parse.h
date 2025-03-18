#pragma once

#include "captcha_gen.h"

#include <antirobot/lib/error.h>

#include <util/generic/string.h>
#include <util/stream/mem.h>

namespace NAntiRobot {
    TErrorOr<TCaptcha> TryParseApiCaptchaResponse(TStringBuf jsonResponse);
}
