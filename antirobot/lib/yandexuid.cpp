#include "yandexuid.h"

namespace NAntiRobot {

TStringBuf GetYandexUid(const THttpCookies& cookies) {
    auto range = cookies.EqualRange(TStringBuf("yandexuid"));
    for (auto i = range.first; i != range.second; ++i) {
        if (!i->second.empty()) {
            return IsValidYandexUid(i->second) ? i->second : TStringBuf();
        }
    }
    return TStringBuf();
}

/* checking for regexp: \d{11,20}
 * more information: https://wiki.yandex-team.ru/Cookies/yandexuid#format
 * more information: http://clubs.at.yandex-team.ru/velo/1463
 */
bool IsValidYandexUid(TStringBuf value) {
    return (11 <= value.size() && value.size() <= 20) && AllOf(value, isdigit);
}

}
