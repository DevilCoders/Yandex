#pragma once

#include <util/generic/map.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/types.h>

namespace NBlackbox2 {
    /// Default argument value in function signatures
    extern const TString EMPTY_STR;

    // Forward declaration of BlackBox response classes
    class TResponse;
    class TBulkResponse;
    class TSessionResp;
    class TMultiSessionResp;
    class TLoginResp;

    /// Get regname from BB response
    struct TRegname: public TMoveOnly {
        TRegname(const TResponse* resp);

        operator const TString&() const {
            return Value();
        }
        const TString& Value() const;

        TString Value_;
    };

    /// Get uid information from BB response
    struct TUid: public TMoveOnly {
        TUid(const TResponse* resp);

        const TString& Uid() const;
        bool Hosted() const;

        TString Uid_;
        bool Hosted_;
    };

    /// Get lite account information from BB response
    struct TLiteUid: public TMoveOnly {
        TLiteUid(const TResponse* resp);

        const TString& LiteUid() const;
        bool LiteAccount() const;

        TString LiteUid_;
        bool LiteAccount_; // not used yet, reserved for future
    };

    /// Get PDD user information from BB response
    struct TPDDUserInfo: public TMoveOnly {
        TPDDUserInfo(const TResponse* resp);

        const TString& DomId() const;
        const TString& Domain() const;
        const TString& Mx() const;
        const TString& DomEna() const;
        const TString& CatchAll() const;

        TString DomId_, Domain_, Mx_;

    private:
        TString DomEna_;
        TString CatchAll_;
    };

    /// Get karma information from BB response
    struct TKarmaInfo: public TMoveOnly {
        TKarmaInfo(const TResponse* resp);

        const TString& Karma() const;
        const TString& BanTime() const;
        const TString& KarmaStatus() const;

        TString Karma_, BanTime_;

    private:
        TString KarmaStatus_;
    };

    /// Get display name information from BB response
    struct TDisplayNameInfo: public TMoveOnly {
        TDisplayNameInfo(const TResponse* resp);

        const TString& Name() const;
        bool Social() const;
        const TString& SocProfile() const;
        const TString& SocProvider() const;
        const TString& SocTarget() const;
        const TString& DefaultAvatar() const;

        TString Name_;

        bool Social_;
        TString SocProfile_, SocProvider_, SocTarget_;

    private:
        TString DefaultAvatar_;
    };

    /// Get login information from BB response
    struct TLoginInfo: public TMoveOnly {
        TLoginInfo(const TResponse* resp);

        const TString& Login() const;
        bool HavePassword() const;
        bool HaveHint() const;

        TString Login_;
        bool HavePassword_;

    private:
        bool HaveHint_;
    };

    /// Get email list from BB response
    class TEmailList: public TMoveOnly {
    public:
        /// Email list item
        struct TItem {
            bool Native() const;
            bool Validated() const;
            bool IsDefault() const;
            bool Rpop() const;
            const TString& Address() const;
            const TString& Date() const;

            bool Native_, Validated_, Default_, Rpop_;
            TString Address_;
            TString Date_;

            TItem();
            TItem(const TItem& i);
            ~TItem();
            TItem& operator=(const TItem& i);
            bool operator==(const TItem& i) const;
        };
        typedef TVector<TItem> ListType;

        TEmailList(const TResponse* resp);

        bool Empty() const {
            return List_.empty();
        }
        size_t Size() const {
            return List_.size();
        }

        const TString& GetDefault() const {
            return Default_ ? Default_->Address_ : EMPTY_STR;
        }
        const TItem* GetDefaultItem() const {
            return Default_;
        }
        const ListType& GetEmailItems() const {
            return List_;
        }

    private:
        ListType List_;
        const TItem* Default_; // pointer to default email, can be NULL
    };

    /// Get new session_id from BB sessionid response
    struct TNewSessionId: public TMoveOnly {
        TNewSessionId(const TSessionResp* resp);
        TNewSessionId(const TMultiSessionResp* resp);

        operator const TString&() const {
            return Value();
        }

        const TString& Value() const;
        const TString& Domain() const;
        const TString& Expires() const;
        bool HttpOnly() const;

        TString Value_;
        TString Domain_;
        TString Expires_;
        bool HttpOnly_;
    };

    /// Get authorization info from BB sessionid response
    struct TAuthInfo: public TMoveOnly {
        TAuthInfo(const TSessionResp* resp);

        bool Social() const;
        const TString& Age() const;
        const TString& ProfileId() const;
        bool Secure() const;

        bool Social_;
        TString Age_, ProfileId_;
        bool Secure_;
    };

    /// Get allow_more_users flag from BB sessionid response
    struct TAllowMoreUsers: public TMoveOnly {
        TAllowMoreUsers(const TMultiSessionResp* resp);
        ~TAllowMoreUsers();

        bool AllowMoreUsers() const;

        bool AllowMoreUsers_;
    };

    class TAliasList: public TMoveOnly {
    public:
        struct TItem {
            enum EType {
                Login = 1, // left out for compatibility, correct name: Portal
                Portal = 1,
                Mail = 2,
                NarodMail = 3,
                Lite = 5,
                Social = 6,
                PDD = 7,
                PDDAlias = 8,
                AltDomain = 9,
                Phonish = 10,
                PhoneNumber = 11,
                Mailish = 12,
                Yandexoid = 13,
                KinopoiskId = 15,
                NeoPhonish = 21
            } Type_;
            TString Alias_;

            EType type() const;
            const TString& alias() const;

            TItem();
            TItem(const TItem& i);
            ~TItem();
            TItem& operator=(const TItem& i);
            bool operator==(const TItem& i) const;
        };
        typedef TVector<TItem> TListType;

        TAliasList(const TResponse* resp);

        bool Empty() const {
            return List_.empty();
        }
        size_t Size() const {
            return List_.size();
        }

        const TListType& GetAliases() const {
            return List_;
        }

    private:
        TListType List_;
    };

    class TOAuthInfo: public TMoveOnly {
    public:
        typedef TMap<TString, TString> TMapType;

        TOAuthInfo(const TSessionResp* resp);

        bool Empty() const {
            return Info_.empty();
        }
        size_t Size() const {
            return Info_.size();
        }

        const TMapType& GetInfo() const {
            return Info_;
        }

    private:
        TMapType Info_;
    };

    struct TBruteforcePolicy: public TMoveOnly {
        enum EMode {
            None,
            Captcha,
            DelayMode,
            Expired
        } Mode_;

        TBruteforcePolicy(const TLoginResp* resp);

        int Delay() const;
        EMode Mode() const;

        int Delay_;
    };

    struct TSessionKind: public TMoveOnly {
        enum EKind {
            None = 0,
            Support = 1,
            Stress = 2
        } Kind_;

        TSessionKind(const TSessionResp* resp);
        TSessionKind(const TMultiSessionResp* resp);

        EKind Kind() const;
        const TString& KindName() const;

        TString KindName_;
    };

    struct TAuthId: public TMoveOnly {
        TAuthId(const TSessionResp* resp);
        TAuthId(const TMultiSessionResp* resp);

        operator const TString&() const {
            return AuthId();
        }

        const TString& Time() const;
        const TString& UserIp() const;
        const TString& HostId() const;
        const TString& AuthId() const;

        TString Time_, UserIp_, HostId_, AuthId_;
    };

    struct TConnectionId: public TMoveOnly {
        TConnectionId(const TLoginResp* resp);
        TConnectionId(const TSessionResp* resp);
        TConnectionId(const TMultiSessionResp* resp);

        operator const TString&() const {
            return ConnectionId();
        }
        const TString& ConnectionId() const;

        TString ConnectionId_;
    };

    struct TUserTicket: public TMoveOnly {
        TUserTicket(const TResponse* resp);

        const TString& Get() const {
            return UserTicket_;
        }

    private:
        TString UserTicket_;
    };

    /// Get information about registration completion from BB response
    struct TRegistrationCompletedInfo: public TMoveOnly {
        TRegistrationCompletedInfo(const TResponse* resp);

        bool NeedRegCompleted() const;

    private:
        bool NeedRegCompleted_;
    };
}
