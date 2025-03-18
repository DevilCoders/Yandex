#pragma once

#include <library/cpp/http/cookies/cookies.h>

namespace NAntiRobot {

TStringBuf GetYandexUid(const THttpCookies& cookies);
bool IsValidYandexUid(TStringBuf yandexUidValue);

}
