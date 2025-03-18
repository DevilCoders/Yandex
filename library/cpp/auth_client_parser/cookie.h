#pragma once

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

/*!
 * See README.md for documentation
 */
namespace NAuthClientParser {
    using TUid = ui64;
    using TTs = ui64;
    using TTtl = ui8;
    using TVersion = ui8;
    using TSocialid = ui32;
    using TPwdCheckDelta = i32;

    using TLang = ui16;

    enum class EParseStatus {
        Invalid,
        NoauthValid,
        RegularMayBeValid,
        RegularExpired
    };

    using TSessionFlags = ui64;
    using TAccountFlags = ui64;
    using TKeyValue = TSmallVec<std::pair<ui32, TStringBuf>>;

    class TZeroAllocationCookie;
    class TFullCookie;

    class TSessionInfo {
    public:
        TTtl Ttl = 0;
        TTs Ts = 0;
        TVersion Version = 0;
        TStringBuf Authid;

        bool IsSafe() const;
        bool IsSuspicious() const;
        bool IsStress() const;

    private:
        TSessionFlags SessionFlags_ = 0;
        friend class TZeroAllocationCookie;
    };

    class TSessionInfoExt: public TSessionInfo {
    public:
        TKeyValue Kv;

    private:
        void Clear();
        friend class TZeroAllocationCookie;
        friend class TFullCookie;
    };

    class TUserInfo {
    public:
        TUid Uid = 0;
        TPwdCheckDelta PwdCheckDelta = 0;
        TLang Lang = 1; // 'ru'
        TSocialid SocialId = 0;

        bool IsLite() const;
        bool HavePwd() const;
        bool IsStaff() const;
        bool IsBetatester() const;
        bool IsGlogouted() const;
        bool IsPartnerPddToken() const;
        bool IsSecure() const;

    private:
        void Clear();

    private:
        TAccountFlags AccountFlags_ = 0;
        friend class TZeroAllocationCookie;
    };

    class TUserInfoExt: public TUserInfo {
    public:
        TKeyValue Kv;
    };

    class TZeroAllocationCookie {
    public:
        EParseStatus Parse(TStringBuf cookie, TInstant now = TInstant::Now());

        EParseStatus Status() const {
            return Status_;
        }

        const TSessionInfo& SessionInfo() const {
            Y_ENSURE(Status_ == EParseStatus::RegularMayBeValid ||
                     Status_ == EParseStatus::RegularExpired ||
                     Status_ == EParseStatus::NoauthValid);
            return SessionInfo_;
        }

        const TUserInfo& User() const {
            Y_ENSURE(Status_ == EParseStatus::RegularMayBeValid ||
                     Status_ == EParseStatus::RegularExpired);
            return UserInfo_;
        }

    protected:
        EParseStatus ParseImpl(TStringBuf cookie, TInstant now);
        EParseStatus ParseAuth(TStringBuf cookie, TInstant now);

        virtual EParseStatus ParseV1(TStringBuf cookie, TInstant now);
        EParseStatus ParseV1Impl(TStringBuf cookie, TInstant now, TUserInfo& userInfo);

        virtual EParseStatus ParseV2(TStringBuf cookie, TInstant now);
        EParseStatus ParseV2Impl(TStringBuf cookie, TInstant now, TUserInfo& userInfo);

        EParseStatus ParseV3(TStringBuf cookie, TInstant now);
        virtual bool ProcessSessionBlockV3(TStringBuf sessKvBlock, size_t defaultIdx);
        virtual bool AddUserV3(TStringBuf userKvBlock, TUserInfoExt&& userInfo, bool actuallyAdd = true);
        static bool FillUserInfoV3(TStringBuf userKvBlock, TUserInfoExt& userInfo, bool addKv);

    protected:
        EParseStatus Status_ = EParseStatus::Invalid;
        TSessionInfoExt SessionInfo_;

    private:
        TUserInfo UserInfo_;
    };

    class TFullCookie final: private TZeroAllocationCookie {
    public:
        EParseStatus Parse(TStringBuf cookie, TInstant now = TInstant::Now());

        EParseStatus Status() const {
            return Status_;
        }

        const TSessionInfoExt& SessionInfo() const {
            Y_ENSURE(Status_ == EParseStatus::RegularMayBeValid ||
                     Status_ == EParseStatus::RegularExpired ||
                     Status_ == EParseStatus::NoauthValid);
            return SessionInfo_;
        }

        const TUserInfoExt& DefaultUser() const {
            Y_ENSURE(Status_ == EParseStatus::RegularMayBeValid ||
                     Status_ == EParseStatus::RegularExpired);
            return UserInfo_.at(DefaultIdx_);
        }

        const TSmallVec<TUserInfoExt>& Users() const {
            Y_ENSURE(Status_ == EParseStatus::RegularMayBeValid ||
                     Status_ == EParseStatus::RegularExpired);
            return UserInfo_;
        }

    private:
        EParseStatus ParseV1(TStringBuf cookie, TInstant now) override;
        EParseStatus ParseV2(TStringBuf cookie, TInstant now) override;

        bool AddUserV3(TStringBuf userKvBlock, TUserInfoExt&& userInfo, bool) override;
        bool ProcessSessionBlockV3(TStringBuf sessKvBlock, size_t defaultIdx) override;

    private:
        TSmallVec<TUserInfoExt> UserInfo_;
        size_t DefaultIdx_ = 0;
    };
}
