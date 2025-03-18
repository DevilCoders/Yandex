#include "responseimpl.h"
#include "utils.h"

#include <library/cpp/blackbox2/accessors.h>

namespace NBlackbox2 {
    inline TResponse::TImpl* getImplChecked(const TResponse* resp) {
        if (!resp)
            throw TFatalError(NSessionCodes::INVALID_PARAMS, "NULL Response pointer");

        return resp->GetImpl();
    }

    TRegname::TRegname(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("regname", Value_);
    }

    const TString& TRegname::Value() const {
        return Value_;
    }

    TUid::TUid(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("uid", Uid_);

        long int hosted;
        if (p->GetIfExists("uid/@hosted", hosted) && hosted)
            Hosted_ = true;
        else
            Hosted_ = false;
    }

    const TString& TUid::Uid() const {
        return Uid_;
    }

    bool TUid::Hosted() const {
        return Hosted_;
    }

    TLiteUid::TLiteUid(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("liteuid", LiteUid_);
    }

    const TString& TLiteUid::LiteUid() const {
        return LiteUid_;
    }

    bool TLiteUid::LiteAccount() const {
        return LiteAccount_;
    }

    TPDDUserInfo::TPDDUserInfo(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("uid/@domid", DomId_);

        TString tmp_domain, decoded;
        p->GetIfExists("uid/@domain", tmp_domain);

        if (!tmp_domain.empty())
            Domain_ = NIdn::Decode(tmp_domain, decoded);

        p->GetIfExists("uid/@mx", Mx_);

        TString ena, catchall;
        p->GetIfExists("uid/@domain_ena", ena);
        p->GetIfExists("uid/@catch_all", catchall);

        if (!ena.empty() || !catchall.empty()) {
            DomEna_.swap(ena);
            CatchAll_.swap(catchall);
        }
    }

    const TString& TPDDUserInfo::DomId() const {
        return DomId_;
    }

    const TString& TPDDUserInfo::Domain() const {
        return Domain_;
    }

    const TString& TPDDUserInfo::Mx() const {
        return Mx_;
    }

    const TString& TPDDUserInfo::DomEna() const {
        return DomEna_;
    }

    const TString& TPDDUserInfo::CatchAll() const {
        return CatchAll_;
    }

    TKarmaInfo::TKarmaInfo(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("karma", Karma_);
        p->GetIfExists("karma/@allow-until", BanTime_);

        TString k_status;
        if (p->GetIfExists("karma_status", k_status) &&
            !k_status.empty()) {
            KarmaStatus_.swap(k_status);
        }
    }

    const TString& TKarmaInfo::Karma() const {
        return Karma_;
    }

    const TString& TKarmaInfo::BanTime() const {
        return BanTime_;
    }

    const TString& TKarmaInfo::KarmaStatus() const {
        return KarmaStatus_;
    }

    TDisplayNameInfo::TDisplayNameInfo(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("display_name/name", Name_);

        TString dummy;
        Social_ = p->GetIfExists("display_name/social", dummy);

        if (Social_) {
            p->GetIfExists("display_name/social/profile_id", SocProfile_);
            p->GetIfExists("display_name/social/provider", SocProvider_);
            p->GetIfExists("display_name/social/redirect_target", SocTarget_);
        }

        TString def_avatar;
        if (p->GetIfExists("display_name/avatar/default", def_avatar) &&
            !def_avatar.empty()) {
            DefaultAvatar_.swap(def_avatar);
        }
    }

    const TString& TDisplayNameInfo::Name() const {
        return Name_;
    }

    bool TDisplayNameInfo::Social() const {
        return Social_;
    }

    const TString& TDisplayNameInfo::SocProfile() const {
        return SocProfile_;
    }

    const TString& TDisplayNameInfo::SocProvider() const {
        return SocProvider_;
    }

    const TString& TDisplayNameInfo::SocTarget() const {
        return SocTarget_;
    }

    const TString& TDisplayNameInfo::DefaultAvatar() const {
        return DefaultAvatar_;
    }

    TLoginInfo::TLoginInfo(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("login", Login_);

        long int passwd;
        if (p->GetIfExists("have_password", passwd) && passwd)
            HavePassword_ = true;
        else
            HavePassword_ = false;

        bool have_hint;
        long int hint;
        if (p->GetIfExists("have_hint", hint) && hint)
            have_hint = true;
        else
            have_hint = false;

        HaveHint_ = have_hint;
    }

    const TString& TLoginInfo::Login() const {
        return Login_;
    }

    bool TLoginInfo::HavePassword() const {
        return HavePassword_;
    }

    bool TLoginInfo::HaveHint() const {
        return HaveHint_;
    }

    TEmailList::TItem::TItem() {
    }

    TEmailList::TItem::TItem(const TItem& i)
        : Native_(i.Native_)
        , Validated_(i.Validated_)
        , Default_(i.Default_)
        , Rpop_(i.Rpop_)
        , Address_(i.Address_)
        , Date_(i.Date_)
    {
    }

    TEmailList::TItem::~TItem() {
    }

    TEmailList::TItem& TEmailList::TItem::operator=(const TEmailList::TItem& i) {
        Native_ = i.Native_;
        Validated_ = i.Validated_;
        Default_ = i.Default_;
        Rpop_ = i.Rpop_;
        Address_ = i.Address_;
        Date_ = i.Date_;

        return *this;
    }

    bool TEmailList::TItem::operator==(const TEmailList::TItem& i) const {
        return Address_ == i.Address_;
    }

    bool TEmailList::TItem::Native() const {
        return Native_;
    }

    bool TEmailList::TItem::Validated() const {
        return Validated_;
    }

    bool TEmailList::TItem::IsDefault() const {
        return Default_;
    }

    bool TEmailList::TItem::Rpop() const {
        return Rpop_;
    }

    const TString& TEmailList::TItem::Address() const {
        return Address_;
    }

    const TString& TEmailList::TItem::Date() const {
        return Date_;
    }

    TEmailList::TEmailList(const TResponse* resp)
        : Default_(NULL)
    {
        TResponse::TImpl* p = getImplChecked(resp);

        xmlConfig::Parts mails(p->GetParts("address-list/address"));
        List_.resize(mails.Size());

        long int val;
        for (int i = 0; i < mails.Size(); ++i) {
            xmlConfig::Part item(mails[i]);

            TString tmp_addr(item.asString()), decoded;

            List_[i].Address_ = NIdn::DecodeAddr(tmp_addr, decoded);

            item.GetIfExists("@born-date", List_[i].Date_);

            List_[i].Native_ = item.GetIfExists("@native", val) && val;
            List_[i].Validated_ = item.GetIfExists("@validated", val) && val;
            List_[i].Rpop_ = item.GetIfExists("@rpop", val) && val;

            if (item.GetIfExists("@default", val) && val) {
                List_[i].Default_ = true;
                Default_ = &List_[i];
            } else
                List_[i].Default_ = false;
        }
    }

    namespace {
        void readCookieData(const TResponse* resp, const TString& name,
                            TString& value, TString& domain, TString& expires, bool& httpOnly) {
            TResponse::TImpl* p = getImplChecked(resp);

            p->GetIfExists(name, value);
            p->GetIfExists(name + "/@domain", domain);
            p->GetIfExists(name + "/@expires", expires);

            long int val;
            httpOnly = p->GetIfExists(name + "/@http-only", val) && val;
        }
    }
    TNewSessionId::TNewSessionId(const TSessionResp* resp) {
        readCookieData(resp, "new-session", Value_, Domain_, Expires_, HttpOnly_);
    }

    TNewSessionId::TNewSessionId(const TMultiSessionResp* resp) {
        readCookieData(resp, "new-session", Value_, Domain_, Expires_, HttpOnly_);
    }

    const TString& TNewSessionId::Value() const {
        return Value_;
    }

    const TString& TNewSessionId::Domain() const {
        return Domain_;
    }

    const TString& TNewSessionId::Expires() const {
        return Expires_;
    }

    bool TNewSessionId::HttpOnly() const {
        return HttpOnly_;
    }

    TAuthInfo::TAuthInfo(const TSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("auth/password_verification_age", Age_);

        TString dummy;
        Social_ = p->GetIfExists("auth/social", dummy);

        if (Social_)
            p->GetIfExists("auth/social/profile_id", ProfileId_);

        long int val;
        Secure_ = p->GetIfExists("auth/secure", val) && val;
    }

    bool TAuthInfo::Social() const {
        return Social_;
    }

    const TString& TAuthInfo::Age() const {
        return Age_;
    }

    const TString& TAuthInfo::ProfileId() const {
        return ProfileId_;
    }

    bool TAuthInfo::Secure() const {
        return Secure_;
    }

    TAllowMoreUsers::TAllowMoreUsers(const TMultiSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        long int val;
        AllowMoreUsers_ = p->GetIfExists("allow_more_users", val) && val;
    }

    TAllowMoreUsers::~TAllowMoreUsers() {
    }

    bool TAllowMoreUsers::AllowMoreUsers() const {
        return AllowMoreUsers_;
    }

    TAliasList::TItem::TItem() {
    }

    TAliasList::TItem::TItem(const TItem& i)
        : Type_(i.Type_)
        , Alias_(i.Alias_)
    {
    }

    TAliasList::TItem::~TItem() {
    }

    TAliasList::TItem& TAliasList::TItem::operator=(const TItem& i) {
        Type_ = i.Type_;
        Alias_ = i.Alias_;

        return *this;
    }

    bool TAliasList::TItem::operator==(const TItem& i) const {
        return (Type_ == i.Type_) && (Alias_ == i.Alias_);
    }

    TAliasList::TItem::EType TAliasList::TItem::type() const {
        return Type_;
    }

    const TString& TAliasList::TItem::alias() const {
        return Alias_;
    }

    TAliasList::TAliasList(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        xmlConfig::Parts aliases(p->GetParts("aliases/alias"));
        List_.resize(aliases.Size());

        long int val;
        for (int i = 0; i < aliases.Size(); ++i) {
            xmlConfig::Part item(aliases[i]);

            val = 0;
            item.GetIfExists("@type", val);

            List_[i].Type_ = static_cast<TItem::EType>(val);
            List_[i].Alias_ = item.asString();
        }
    }

    TOAuthInfo::TOAuthInfo(const TSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        xmlConfig::Parts oauth(p->GetParts("OAuth/*"));

        for (int i = 0; i < oauth.Size(); ++i) {
            xmlConfig::Part item(oauth[i]);
            Info_[item.GetName()] = item.asString();
        }
    }

    TBruteforcePolicy::TBruteforcePolicy(const TLoginResp* resp)
        : Mode_(None)
        , Delay_(0)
    {
        TResponse::TImpl* p = getImplChecked(resp);

        long int val;

        TString tmp;
        if (p->GetIfExists("bruteforce_policy/captcha", tmp))
            Mode_ = Captcha;
        else if (p->GetIfExists("bruteforce_policy/password_expired", tmp))
            Mode_ = Expired;
        else if (p->GetIfExists("bruteforce_policy/delay", val)) {
            Mode_ = DelayMode;
            Delay_ = static_cast<int>(val);
        }
    }

    int TBruteforcePolicy::Delay() const {
        return Delay_;
    }

    TBruteforcePolicy::EMode TBruteforcePolicy::Mode() const {
        return Mode_;
    }

    TSessionKind::TSessionKind(const TSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        long int k;

        if (p->GetIfExists("special/@id", k)) {
            Kind_ = static_cast<EKind>(k);
            p->GetIfExists("special", KindName_);
        } else
            Kind_ = None;
    }

    TSessionKind::TSessionKind(const TMultiSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        long int k;

        if (p->GetIfExists("special/@id", k)) {
            Kind_ = static_cast<EKind>(k);
            p->GetIfExists("special", KindName_);
        } else
            Kind_ = None;
    }

    TSessionKind::EKind TSessionKind::Kind() const {
        return Kind_;
    }

    const TString& TSessionKind::KindName() const {
        return KindName_;
    }

    TAuthId::TAuthId(const TSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("authid/@time", Time_);
        p->GetIfExists("authid/@ip", UserIp_);
        p->GetIfExists("authid/@host", HostId_);
        p->GetIfExists("authid", AuthId_);
    }

    TAuthId::TAuthId(const TMultiSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("authid/@time", Time_);
        p->GetIfExists("authid/@ip", UserIp_);
        p->GetIfExists("authid/@host", HostId_);
        p->GetIfExists("authid", AuthId_);
    }

    const TString& TAuthId::Time() const {
        return Time_;
    }

    const TString& TAuthId::UserIp() const {
        return UserIp_;
    }

    const TString& TAuthId::HostId() const {
        return HostId_;
    }

    const TString& TAuthId::AuthId() const {
        return AuthId_;
    }

    TConnectionId::TConnectionId(const TLoginResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("connection_id", ConnectionId_);
    }

    TConnectionId::TConnectionId(const TSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("connection_id", ConnectionId_);
    }

    TConnectionId::TConnectionId(const TMultiSessionResp* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        p->GetIfExists("connection_id", ConnectionId_);
    }

    const TString& TConnectionId::ConnectionId() const {
        return ConnectionId_;
    }

    TUserTicket::TUserTicket(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);
        p->GetIfExists("user_ticket", UserTicket_);
        if (UserTicket_.empty()) {
            throw TFatalError(NSessionCodes::INVALID_PARAMS, "UserTicket does not exist");
        }
    }

    TRegistrationCompletedInfo::TRegistrationCompletedInfo(const TResponse* resp) {
        TResponse::TImpl* p = getImplChecked(resp);

        long int needRegCompleted = false;
        if (p->GetIfExists("is_reg_completion_recommended", needRegCompleted) && needRegCompleted) {
            NeedRegCompleted_ = true;
        } else {
            NeedRegCompleted_ = false;
        }
    }

    bool TRegistrationCompletedInfo::NeedRegCompleted() const {
        return NeedRegCompleted_;
    }
}
