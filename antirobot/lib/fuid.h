#pragma once

#include "ar_utils.h"

#include <library/cpp/fuid/fuid.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>

class IInputStream;
class IOutputStream;
class THttpCookies;

namespace NAntiRobot {
    class TFlashCookie {
    public:
        static constexpr TStringBuf Name = "fuid01"_sb;

    public:
        inline TFlashCookie()
            : Fuid({0, 0})
        {
        }

        inline TFlashCookie* Parse(const TStringBuf& uid) {
            if (Parse(uid, *this)) {
                return this;
            }

            return nullptr;
        }

        TFlashCookie* ParseCookies(const THttpCookies& cookies);
        static bool Parse(const TStringBuf& uid, TFlashCookie& res);

        ui64 Id() const;
        static TFlashCookie FromId(ui64 id);

        TInstant Time() const;

        void In(IInputStream& in);
        void Out(IOutputStream& out) const;
    private:
        TFuidChecker::TFuid Fuid;
    };
}
