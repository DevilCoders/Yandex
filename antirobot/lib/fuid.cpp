#include "fuid.h"

#include <library/cpp/http/cookies/cookies.h>

#include <util/stream/output.h>
#include <util/string/cast.h>

namespace NAntiRobot {

bool TFlashCookie::Parse(const TStringBuf& uid, TFlashCookie& res) {
    return FuidChecker().Parse(uid, res.Fuid);
}

TFlashCookie* TFlashCookie::ParseCookies(const THttpCookies& cookies) {
    return Parse(cookies.Get(Name));
}


ui64 TFlashCookie::Id() const {
    return static_cast<ui64>(Fuid.Rand) << 32 | Fuid.Time;
}

TFlashCookie TFlashCookie::FromId(ui64 id) {
    TFlashCookie res;
    res.Fuid.Rand = id >> 32;
    res.Fuid.Time = id & Max<ui32>();
    return res;
}

TInstant TFlashCookie::Time() const {
    return TInstant::Seconds(Fuid.Time);
}

void TFlashCookie::In(IInputStream& in) {
    Fuid.Rand = FromString<ui32>(in.ReadTo('-'));
    in >> Fuid.Time;
}

void TFlashCookie::Out(IOutputStream& out) const {
    out << Fuid.Rand
        << "-"sv
        << Fuid.Time
        ;
}

} // namespace NAntiRobot

template <>
void In<NAntiRobot::TFlashCookie>(IInputStream& in, NAntiRobot::TFlashCookie& fuid) {
    fuid.In(in);
}

template <>
void Out<NAntiRobot::TFlashCookie>(IOutputStream& out, const NAntiRobot::TFlashCookie& fuid) {
   fuid.Out(out);
}
