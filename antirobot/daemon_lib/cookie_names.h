#pragma once

#include <antirobot/lib/ar_utils.h>

#include <util/generic/strbuf.h>

#include <array>

namespace NAntiRobot {
    constexpr std::array CookieName{
        "fyandex"_sb,
        "L"_sb,
        "mode"_sb,
        "my"_sb,
        "Session_id"_sb,
        "Secure_session_id"_sb,
        "spravka"_sb,       // ??
        "Virtual_id"_sb,
        "yabs-frequency"_sb,
        "yandex_gid"_sb,
        "yandex_login"_sb,
        "yandex_mist_user"_sb,
        "yandexmarket"_sb,
        "yandexuid"_sb,
        "yp"_sb,
        "ys"_sb,
        "YX_SEARCHPREFS"_sb,
        "fuid01"_sb,
        "be_mobile"_sb,
        "S"_sb,
        "aw"_sb,
        "YX_SHOW_RELEVANCE"_sb,
        "YX_SHOW_STUFF"_sb,
        "ASSESS_SPAM"_sb,
        "YX_HIDE_SPAMBOX"_sb,
        "YX_HIDE_FEEDBACK"_sb,
        "YX_DBGWZR"_sb,
        "YX_SBH"_sb,
        "YX_NMYS"_sb,
        "YX_NMSS"_sb,
        "YX_NMRL"_sb,
    };

    constexpr std::array CacherCookieName{
        "L"_sb,
        "my"_sb,
        "Session_id"_sb,
        "yabs-frequency"_sb,
        "yandex_login"_sb,
        "yandexuid"_sb,
        "ys"_sb,
        "fuid01"_sb,
    };
} // namespace NAntiRobot
