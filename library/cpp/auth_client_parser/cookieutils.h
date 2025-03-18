#pragma once

#include "cookie.h"

namespace NAuthClientParser {
    namespace NPrivate {
        TDuration ExpiresIn(TTtl ttl) noexcept;
        bool CheckExpires(TTtl ttl, TTs ts, TInstant now) noexcept;

        bool ParseUid(TStringBuf uidStr, TUid& uid, bool& lite);
        bool ParseAuthId(TStringBuf str);

        EParseStatus ParseNoAuth(TStringBuf cookie, TTs& ts);
        EParseStatus ParseHeaderV1V2(const TStringBuf cookie,
                                     TInstant now,
                                     TTtl& ttl,
                                     TUid& uid,
                                     bool& lite,
                                     TTs& ts,
                                     size_t& len);
        EParseStatus ParseHeaderV3(const TStringBuf cookie,
                                   TInstant now,
                                   TTtl& ttl,
                                   TTs& ts,
                                   size_t& defIdx,
                                   TSessionFlags& sflags,
                                   TStringBuf& authid,
                                   TStringBuf& kvBlock);
        bool ParseFooterV1(const TStringBuf cookie, size_t& len);
        bool ParseFooterV2(const TStringBuf cookie,
                           bool& safe,
                           TLang& lang,
                           bool& havePwd,
                           TPwdCheckDelta& pwdCheckDelta,
                           size_t& len);
        bool ParseFooterV3(TStringBuf cookie);

        bool ParseUser(TStringBuf cookie,
                       TUid& uid,
                       TPwdCheckDelta& pwdCheckDelta,
                       TAccountFlags& aflags,
                       TStringBuf& kvBlock);

        bool CheckKV(TStringBuf cookie);

        bool ParseSids(TStringBuf cookie,
                       TSessionFlags& sflags,
                       TAccountFlags& aflags,
                       TStringBuf& authid,
                       TSocialid& socialId);

        bool ParseLang(const TStringBuf lang, TLang& out);
    }
}
