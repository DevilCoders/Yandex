#pragma once

#include <antirobot/lib/ar_utils.h>

#include <yweb/webdaemons/icookiedaemon/icookie_lib/utils/consts.h>

#include <util/generic/strbuf.h>

#include <array>

namespace NAntiRobot {
    constexpr std::array HttpHeaderName{
        "Accept"_sb,
        "Accept-Charset"_sb,
        "Accept-Encoding"_sb,
        "Accept-Language"_sb,
        "Accept-Ranges"_sb,
        "Authorization"_sb,
        "Cache-Control"_sb,
        "Cookie"_sb,
        "Connection"_sb,
        "Content-Length"_sb,
        "Content-Type"_sb,
        "Date"_sb,
        "Expect"_sb,
        "From"_sb,
        "Host"_sb,
        "If-Match"_sb,
        "If-Modified-Since"_sb,
        "If-None-Match"_sb,
        "If-Range"_sb,
        "If-Unmodified-Since"_sb,
        "Max-Forwards"_sb,
        "Pragma"_sb,
        "Proxy-Authorization"_sb,
        "Range"_sb,
        "Referer"_sb,
        "TE"_sb,
        "Upgrade"_sb,
        "User-Agent"_sb,
        "Via"_sb,
        "Warning"_sb,
        "X-Forwarded-For"_sb,
    };

    /* Internal headers.
     * They are not factors, but should not calculated as unknwon
     * Keep this list up to date.
     */
    constexpr std::array HttpInternalHeaderName{
        "X-Req-Id"_sb,
        "X-Forwarded-For-Y"_sb,
        "X-Source-Port-Y"_sb,
        "X-Yandex-Ja3"_sb,
        "X-Yandex-Ja4"_sb,
        "X-Start-Time"_sb,
        TStringBuf(NIcookie::ICOOKIE_DECRYPTED_HEADER),
    };

    /*
     *  Headers using in Cacher
     */
    constexpr std::array CacherHttpHeaderName{
        "Accept-Encoding"_sb,
        "Accept-Language"_sb,
        "Authorization"_sb,
        "Cache-Control"_sb,
        "Cookie"_sb,
        "Connection"_sb,
        "Content-Length"_sb,
        "Content-Type"_sb,
        "Referer"_sb,
        "User-Agent"_sb,
        "X-Forwarded-For"_sb,
    };

} // namespace NAntiRobot
