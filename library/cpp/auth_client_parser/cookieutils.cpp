#include "cookieutils.h"

#include "cookie.h"

#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/string/cast.h>
#include <util/system/compat.h>

namespace NAuthClientParser {
    namespace NPrivate {
        TDuration ExpiresIn(TTtl ttl) noexcept {
            switch (ttl) {
                case 0:
                case 2: // session cookie, 2 hours
                    return TDuration::Seconds(2 * 60 * 60);
                case 1:
                case 5: // permanent cookie, 3 months
                    return TDuration::Seconds(90 * 24 * 60 * 60);
                case 3: // two-weeks cookie, 2 weeks (surprise!)
                    return TDuration::Seconds(14 * 24 * 60 * 60);
                case 4: // restricted cookie, 10 minutes
                    return TDuration::Seconds(4 * 60 * 60);
                default: // unknown yet
                    return TDuration::Seconds(0);
            }
        }

        bool CheckExpires(TTtl ttl, TTs ts, TInstant now) noexcept {
            if (ttl > 5) {
                return false; // unknown ttl future extension
            }
            if (now - TInstant::Seconds(ts) > ExpiresIn(ttl)) {
                return true; // expires
            }
            return false;
        }

        bool ParseUid(TStringBuf uidStr, TUid& uid, bool& lite) {
            if (uidStr.StartsWith('*')) {
                uidStr.Skip(1);
                lite = true;
            } else {
                lite = false;
            }

            return TryIntFromString<10>(uidStr, uid);
        }

        bool ParseAuthId(TStringBuf str) {
            TTs ts;
            ui32 hostid;
            return TryIntFromString<10>(str.NextTok(':'), ts) &&
                   str.NextTok(':') &&
                   TryIntFromString<16>(str, hostid);
        }

        EParseStatus ParseNoAuth(TStringBuf cookie, TTs& ts) {
            if (!cookie.StartsWith("noauth:")) {
                return EParseStatus::RegularMayBeValid;
            }
            return TryIntFromString<10>(cookie.Skip(7), ts) ? EParseStatus::NoauthValid : EParseStatus::Invalid;
        }

        EParseStatus ParseHeaderV1V2(const TStringBuf cookie,
                                     TInstant now,
                                     TTtl& ttl,
                                     TUid& uid,
                                     bool& lite,
                                     TTs& ts,
                                     size_t& len) {
            TStringBuf tmp = cookie;
            time_t delta = 0;

            if (!TryIntFromString<10>(tmp.NextTok('.'), ts) ||
                !TryIntFromString<10>(tmp.NextTok('.'), delta) ||
                !TryIntFromString<10>(tmp.NextTok('.'), ttl) ||
                !ParseUid(tmp.NextTok('.'), uid, lite)) {
                return EParseStatus::Invalid;
            }

            len = cookie.size() - tmp.size();

            return CheckExpires(ttl, ts, now) ? EParseStatus::RegularExpired : EParseStatus::RegularMayBeValid;
        }

        EParseStatus ParseHeaderV3(const TStringBuf cookie,
                                   TInstant now,
                                   TTtl& ttl,
                                   TTs& ts,
                                   size_t& defIdx,
                                   TSessionFlags& sflags,
                                   TStringBuf& authid,
                                   TStringBuf& kvBlock) {
            TStringBuf tmp = cookie;
            if (!TryIntFromString<10>(tmp.NextTok('.'), ts) ||
                !TryIntFromString<10>(tmp.NextTok('.'), ttl) ||
                !TryIntFromString<16>(tmp.NextTok('.'), defIdx)) {
                return EParseStatus::Invalid;
            }

            authid = tmp.NextTok('.');

            if (!ParseAuthId(authid) ||
                !TryIntFromString<16>(tmp.NextTok('.'), sflags)) {
                return EParseStatus::Invalid;
            }
            kvBlock = tmp;

            return CheckExpires(ttl, ts, now) ? EParseStatus::RegularExpired : EParseStatus::RegularMayBeValid;
        }

        bool ParseFooterV1(const TStringBuf cookie, size_t& len) {
            TStringBuf tmp = cookie;
            if (!tmp.RNextTok('.') || // sign
                !tmp.RNextTok('.') || // rnd
                !tmp.RNextTok('.')) { // keyid
                return false;
            }

            len = cookie.size() - tmp.size();
            return true;
        }

        bool ParseFooterV2(const TStringBuf cookie,
                           bool& safe,
                           TLang& lang,
                           bool& havePwd,
                           TPwdCheckDelta& pwdCheckDelta,
                           size_t& len) {
            if (!ParseFooterV1(cookie, len)) {
                return false;
            }

            TStringBuf tmp = TStringBuf(cookie).Trunc(cookie.size() - len);
            TStringBuf pwd = tmp.RNextTok('.');
            if (pwd && !TryIntFromString<10>(pwd, pwdCheckDelta)) {
                return false;
            }

            TStringBuf havePwd_ = tmp.RNextTok('.');
            if (havePwd_) {
                ui16 pwdNum;
                if (!TryIntFromString<10>(havePwd_, pwdNum) || (pwdNum != 0 && pwdNum != 1)) {
                    return false;
                }
                havePwd = pwdNum;
            }

            if (!ParseLang(tmp.RNextTok('.'), lang)) {
                return false;
            }

            TStringBuf isSafe = tmp.RNextTok('.');
            if (isSafe) {
                ui16 safeNum;
                if (!TryIntFromString<10>(isSafe, safeNum) || (safeNum != 0 && safeNum != 1)) {
                    return false;
                }
                safe = safeNum;
            }

            len = cookie.size() - tmp.size();
            return true;
        }

        bool ParseFooterV3(TStringBuf cookie) {
            return cookie.NextTok('.') && // keyid
                   cookie.NextTok('.') && // rnd
                   cookie.NextTok('.') && // sig
                   !cookie;
        }

        bool ParseUser(TStringBuf cookie,
                       TUid& uid,
                       TPwdCheckDelta& pwdCheckDelta,
                       TAccountFlags& aflags,
                       TStringBuf& kvBlock) {
            if (!TryIntFromString<10>(cookie.NextTok('.'), uid) ||
                !TryIntFromString<10>(cookie.NextTok('.'), pwdCheckDelta) ||
                !TryIntFromString<16>(cookie.NextTok('.'), aflags)) {
                return false;
            }

            kvBlock = cookie;
            return true;
        }

        bool CheckKV(TStringBuf cookie) {
            if (cookie && (cookie[0] == '.' || cookie.back() == '.')) {
                return false;
            }

            while (cookie) {
                ui32 val;
                TStringBuf k, v;
                if (!cookie.NextTok('.').TrySplit(':', k, v) ||
                    !TryIntFromString<16>(k, val)) {
                    return false;
                }
            }

            return true;
        }

        bool ParseSids(TStringBuf cookie,
                       TSessionFlags& sflags,
                       TAccountFlags& aflags,
                       TStringBuf& authid,
                       TSocialid& socialId) {
            if (cookie == "0") {
                return true;
            }

            if (cookie && (cookie[0] == '.' || cookie.back() == '.')) {
                return false;
            }

            while (cookie) {
                TStringBuf k, val;
                ui32 sid;
                if (!cookie.NextTok('.').TrySplit(':', k, val) ||
                    !TryIntFromString<10>(k, sid)) {
                    return false;
                }

                switch (sid) {
                    case 1:             // special flag, probably stress
                        if (val == "2") // 1:2 - stress
                            sflags |= 0x100;
                        break;
                    case 8: // authid
                        authid = val;
                        break;
                    case 58: // social_id
                        if (!TryIntFromString<10>(val, socialId)) {
                            return false;
                        }
                        break;
                    case 668:           // betatester
                        if (val == "1") // 668:1 - betatester
                            aflags |= 0x200;
                        break;
                    case 669:           // staff
                        if (val == "1") // 669:1 - staff
                            aflags |= 0x100;
                        break;
                }
            }
            return true;
        }

        struct TLangMap {
            TLangMap()
                : LangMap({
                      {"ru", 1},
                      {"uk", 2},
                      {"en", 3},
                      {"kk", 4},
                      {"be", 5},
                      {"tt", 6},
                      {"az", 7},
                      {"tr", 8},
                      {"hy", 9},
                      {"ka", 10},
                      {"ro", 11},
                      {"de", 12},
                      {"id", 13},
                      {"zh", 14},
                      {"es", 15},
                      {"pt", 16},
                      {"fr", 17},
                      {"it", 18},
                      {"ja", 19},
                      {"br", 20},
                      {"cs", 21},
                      {"fi", 22},
                      {"uz", 23},
                  })
            {
            }

            const TMap<TString, TLang> LangMap;
        };

        bool ParseLang(const TStringBuf lang, TLang& out) {
            if (!lang) {
                out = 1;
                return true;
            }

            if (TryIntFromString<10>(lang, out)) {
                return true;
            }

            const TLangMap* map = Singleton<TLangMap>();
            auto it = map->LangMap.find(lang);
            if (it != map->LangMap.end()) {
                out = it->second;
                return true;
            }

            return false;
        }
    }
}
