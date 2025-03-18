#include "cookie.h"

#include "cookieutils.h"

#include <util/string/split.h>
#include <util/system/compat.h>
#include <util/system/compiler.h>

#include <string.h>

namespace NAuthClientParser {
    EParseStatus TZeroAllocationCookie::Parse(TStringBuf cookie, TInstant now) {
        SessionInfo_.Clear();
        UserInfo_.Clear();

        return ParseImpl(cookie, now);
    }

    EParseStatus TZeroAllocationCookie::ParseImpl(TStringBuf cookie, TInstant now) {
        try {
            EParseStatus res = NPrivate::ParseNoAuth(cookie, SessionInfo_.Ts);
            if (EParseStatus::NoauthValid == res) {
                SessionInfo_.Version = 1;
            }
            Status_ = EParseStatus::RegularMayBeValid == res
                          ? ParseAuth(cookie, now)
                          : res;
        } catch (const yexception&) { // impossible
            Status_ = EParseStatus::Invalid;
        }
        return Status_;
    }

    EParseStatus TZeroAllocationCookie::ParseAuth(TStringBuf cookie, TInstant now) {
        if (cookie.size() < 10) {
            return EParseStatus::Invalid;
        }

        if (cookie[1] == ':') {
            const char ver = cookie[0];
            cookie.Skip(2);
            switch (ver) {
                case '2':
                    SessionInfo_.Version = 2;
                    return ParseV2(cookie, now);
                case '3':
                    SessionInfo_.Version = 3;
                    return ParseV3(cookie, now);
                default:
                    return EParseStatus::Invalid;
            }
        }

        SessionInfo_.Version = 1;
        return ParseV1(cookie, now);
    }

    EParseStatus TZeroAllocationCookie::ParseV1(TStringBuf cookie, TInstant now) {
        return ParseV1Impl(cookie, now, UserInfo_);
    }

    EParseStatus TZeroAllocationCookie::ParseV1Impl(TStringBuf cookie, TInstant now, TUserInfo& userInfo) {
        bool lite;
        size_t len = 0;

        EParseStatus res = NPrivate::ParseHeaderV1V2(cookie,
                                                     now,
                                                     SessionInfo_.Ttl,
                                                     userInfo.Uid,
                                                     lite,
                                                     SessionInfo_.Ts,
                                                     len);
        if (EParseStatus::Invalid == res) {
            return EParseStatus::Invalid;
        }

        if (!NPrivate::ParseFooterV1(cookie.Skip(len), len)) {
            return EParseStatus::Invalid;
        }

        if (!NPrivate::ParseSids(cookie.Trunc(cookie.size() - len),
                                 SessionInfo_.SessionFlags_,
                                 userInfo.AccountFlags_,
                                 SessionInfo_.Authid,
                                 userInfo.SocialId))
        {
            return EParseStatus::Invalid;
        }

        userInfo.AccountFlags_ |= 0x02; // have_pwd
        if (lite) {
            userInfo.AccountFlags_ |= 0x01; // is_lite
        }

        return res;
    }

    EParseStatus TZeroAllocationCookie::ParseV2(TStringBuf cookie, TInstant now) {
        return ParseV2Impl(cookie, now, UserInfo_);
    }

    EParseStatus TZeroAllocationCookie::ParseV2Impl(TStringBuf cookie, TInstant now, TUserInfo& userInfo) {
        bool lite;
        size_t len = 0;

        EParseStatus res = NPrivate::ParseHeaderV1V2(cookie,
                                                     now,
                                                     SessionInfo_.Ttl,
                                                     userInfo.Uid,
                                                     lite,
                                                     SessionInfo_.Ts,
                                                     len);
        if (EParseStatus::Invalid == res) {
            return EParseStatus::Invalid;
        }

        bool safe = false;
        bool havePwd = false;
        userInfo.PwdCheckDelta = -1;

        if (!NPrivate::ParseFooterV2(cookie.Skip(len),
                                     safe,
                                     userInfo.Lang,
                                     havePwd,
                                     userInfo.PwdCheckDelta,
                                     len))
        {
            return EParseStatus::Invalid;
        }

        if (!NPrivate::ParseSids(cookie.Trunc(cookie.size() - len),
                                 SessionInfo_.SessionFlags_,
                                 userInfo.AccountFlags_,
                                 SessionInfo_.Authid,
                                 userInfo.SocialId))
        {
            return EParseStatus::Invalid;
        }

        if (safe) {
            SessionInfo_.SessionFlags_ |= 0x01;
        }

        if (lite) {
            userInfo.AccountFlags_ |= 0x01; // is_lite
        }
        if (havePwd) {
            userInfo.AccountFlags_ |= 0x02; // have_pwd
        }

        return res;
    }

    EParseStatus TZeroAllocationCookie::ParseV3(TStringBuf cookie, TInstant now) {
        TStringBuf header = cookie.NextTok('|');
        TStringBuf footer = cookie.RNextTok('|');
        if (!header || !cookie || !footer) {
            return EParseStatus::Invalid;
        }

        size_t defaultIndex = 0;
        TStringBuf sessKvBlock;

        EParseStatus res = NPrivate::ParseHeaderV3(header,
                                                   now,
                                                   SessionInfo_.Ttl,
                                                   SessionInfo_.Ts,
                                                   defaultIndex,
                                                   SessionInfo_.SessionFlags_,
                                                   SessionInfo_.Authid,
                                                   sessKvBlock);

        if (EParseStatus::Invalid == res || !ProcessSessionBlockV3(sessKvBlock, defaultIndex)) {
            return EParseStatus::Invalid;
        }

        if (!NPrivate::ParseFooterV3(footer)) {
            return EParseStatus::Invalid;
        }

        size_t numUser = 0;

        while (cookie) {
            TStringBuf cur = cookie.NextTok('|');

            TUserInfoExt userInfo;
            TStringBuf userKvBlock;

            if (!NPrivate::ParseUser(cur,
                                     userInfo.Uid,
                                     userInfo.PwdCheckDelta,
                                     userInfo.AccountFlags_,
                                     userKvBlock))
            {
                return EParseStatus::Invalid;
            }

            if (!AddUserV3(userKvBlock, std::move(userInfo), numUser++ == defaultIndex)) {
                return EParseStatus::Invalid;
            }
        }

        if (numUser <= defaultIndex) {
            return EParseStatus::Invalid;
        }

        return res;
    }

    bool TZeroAllocationCookie::ProcessSessionBlockV3(TStringBuf sessKvBlock, size_t) {
        return NPrivate::CheckKV(sessKvBlock);
    }

    bool TZeroAllocationCookie::AddUserV3(TStringBuf userKvBlock, TUserInfoExt&& userInfo, bool actuallyAdd) {
        if (!actuallyAdd) {
            return true;
        }

        if (!FillUserInfoV3(userKvBlock, userInfo, false)) {
            return false;
        }

        UserInfo_ = std::move(userInfo);
        return true;
    }

    bool TZeroAllocationCookie::FillUserInfoV3(TStringBuf userKvBlock, TUserInfoExt& userInfo, bool addKv) {
        while (userKvBlock) {
            TStringBuf k, v;
            ui32 key;
            if (!userKvBlock.NextTok('.').TrySplit(':', k, v) ||
                !TryIntFromString<16>(k, key)) {
                return false;
            }

            if (key == 0) {
                if (!NPrivate::ParseLang(v, userInfo.Lang)) {
                    return false;
                }
            } else if (key == 1) {
                if (!TryIntFromString<10>(v, userInfo.SocialId) || userInfo.SocialId == 0) {
                    return false;
                }
            }

            if (addKv) {
                userInfo.Kv.push_back({key, v});
            }
        }

        return true;
    }

    bool TSessionInfo::IsSafe() const {
        return SessionFlags_ & 0x01;
    }

    bool TSessionInfo::IsSuspicious() const {
        return SessionFlags_ & 0x02;
    }

    bool TSessionInfo::IsStress() const {
        return SessionFlags_ & 0x100;
    }

    bool TUserInfo::IsLite() const {
        return AccountFlags_ & 0x01;
    }

    bool TUserInfo::HavePwd() const {
        return AccountFlags_ & 0x02;
    }

    bool TUserInfo::IsStaff() const {
        return AccountFlags_ & 0x100;
    }

    bool TUserInfo::IsBetatester() const {
        return AccountFlags_ & 0x200;
    }

    bool TUserInfo::IsGlogouted() const {
        return AccountFlags_ & 0x400;
    }

    bool TUserInfo::IsPartnerPddToken() const {
        return AccountFlags_ & 0x800;
    }

    bool TUserInfo::IsSecure() const {
        return AccountFlags_ & 0x1000;
    }

    void TUserInfo::Clear() {
        *this = TUserInfo();
    }

    EParseStatus TFullCookie::Parse(TStringBuf cookie, TInstant now) {
        SessionInfo_.Clear();
        UserInfo_.clear();

        return ParseImpl(cookie, now);
    }

    EParseStatus TFullCookie::ParseV1(TStringBuf cookie, TInstant now) {
        UserInfo_.push_back(TUserInfoExt());
        return ParseV1Impl(cookie, now, UserInfo_.back());
    }

    EParseStatus TFullCookie::ParseV2(TStringBuf cookie, TInstant now) {
        UserInfo_.push_back(TUserInfoExt());
        return ParseV2Impl(cookie, now, UserInfo_.back());
    }

    bool TFullCookie::ProcessSessionBlockV3(TStringBuf sessKvBlock, size_t defaultIdx) {
        DefaultIdx_ = defaultIdx;

        while (sessKvBlock) {
            TStringBuf k, v;
            ui32 key;
            if (!sessKvBlock.NextTok('.').TrySplit(':', k, v) ||
                !TryIntFromString<16>(k, key)) {
                return false;
            }

            SessionInfo_.Kv.push_back({key, v});
        }
        return true;
    }

    bool TFullCookie::AddUserV3(TStringBuf userKvBlock, TUserInfoExt&& userInfo, bool) {
        if (!FillUserInfoV3(userKvBlock, userInfo, true)) {
            return false;
        }

        UserInfo_.push_back(std::move(userInfo));
        return true;
    }

    void TSessionInfoExt::Clear() {
        *this = TSessionInfoExt();
    }
}
