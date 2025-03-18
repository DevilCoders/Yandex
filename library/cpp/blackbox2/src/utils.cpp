// ====================================================================
//  Miscelaneous library utilities and helpers
// ====================================================================

#include "utils.h"

#include <library/cpp/blackbox2/blackbox2.h>

#include <contrib/libs/libidn/lib/idna.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace NBlackbox2 {
    /// convert hex character to int value
    int hex2int(char c) {
        if ('0' <= c && c <= '9')
            return c - '0';
        if ('a' <= c && c <= 'f')
            return c - 'a' + 10;
        if ('A' <= c && c <= 'F')
            return c - 'A' + 10;

        return -1; // error
    }

    /// convert int value 0..15 to hex char
    char int2hex(int n) {
        if (0 <= n && n <= 9)
            return static_cast<char>('0' + n);
        if (10 <= n && n <= 15)
            return static_cast<char>('A' + n - 10);

        return ' '; // error
    }

    const TString illegal = "!*'();:@&=+$./?#[]%";

    TString URLEncode(const TStringBuf str) {
        const size_t len = str.size();

        TString res;
        res.reserve(len);
        char hex[3] = {'%', ' ', ' '};

        for (size_t i = 0; i < len; ++i) {
            char c = str[i];
            if (!isprint(c) || isspace(c) || illegal.find(c) != TString::npos) {
                hex[1] = int2hex((c >> 4) & 0xF);
                hex[2] = int2hex(c & 0xF);
                res.append(hex, 3);
            } else
                res.append(1, c);
        }

        return res;

        //return CUrl::get().URLEncode(str);
    }

    TString URLDecode(const TStringBuf str) {
        const size_t len = str.size();

        TString res;
        res.reserve(len);

        size_t p, q;
        p = 0;

        do {
            q = str.find('%', p);
            if (q == TString::npos) {
                res.append(str, p, len - p);
                return res;
            }
            if (len - q < 2) // no 2 letters after %
                return "";   // error failed to parse

            res.append(str, p, q - p);

            ++q; // skip %
            int c1 = hex2int(str[q]);
            ++q;
            int c2 = hex2int(str[q]);
            ++q;

            if (c1 >= 0 && c2 >= 0)
                res.append(1, static_cast<char>((c1 << 4) + c2));
            else
                return ""; // failed to parse

            p = q;

        } while (p < len);

        return res;

        //return CUrl::get().URLDecode(str);
    }

    namespace {
        const unsigned char b64_encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        unsigned char b64_decode[256];
        bool b64_init() {
            for (int i = 0; i < 256; ++i)
                b64_decode[i] = 0xff;

            for (int i = 0; i < 64; ++i)
                b64_decode[b64_encode[i]] = static_cast<unsigned char>(i);

            return true;
        }

        bool b64_decode_inited = b64_init();
    }

    TString Bin2base64url(const char* buf, size_t len, bool pad) {
        if (buf == NULL) {
            return TString();
        }

        TString res;
        res.resize(((len + 2) / 3) << 2, 0);

        const unsigned char* pB = reinterpret_cast<const unsigned char*>(buf);
        const unsigned char* pE = reinterpret_cast<const unsigned char*>(buf) + len;
        unsigned char* p = reinterpret_cast<unsigned char*>(const_cast<char*>(res.data()));
        for (; pB + 2 < pE; pB += 3) {
            const unsigned char a = *pB;
            *p++ = b64_encode[(a >> 2) & 0x3F];
            const unsigned char b = *(pB + 1);
            *p++ = b64_encode[((a & 0x3) << 4) | ((b & 0xF0) >> 4)];
            const unsigned char c = *(pB + 2);
            *p++ = b64_encode[((b & 0xF) << 2) | ((c & 0xC0) >> 6)];
            *p++ = b64_encode[c & 0x3F];
        }

        if (pB < pE) {
            const unsigned char a = *pB;
            *p++ = b64_encode[(a >> 2) & 0x3F];
            if (pB == (pE - 1)) {
                *p++ = b64_encode[((a & 0x3) << 4)];
                if (pad) {
                    *p++ = '=';
                }
            } else {
                const unsigned char b = *(pB + 1);
                *p++ = b64_encode[((a & 0x3) << 4) |
                                  (static_cast<int>(b & 0xF0) >> 4)];
                *p++ = b64_encode[((b & 0xF) << 2)];
            }
            if (pad) {
                *p++ = '=';
            }
        }

        res.resize(static_cast<size_t>(p - reinterpret_cast<const unsigned char*>(res.data())));
        return res;
    }

    TString Base64url2bin(const char* buf, size_t len) {
        const unsigned char* bufin = reinterpret_cast<const unsigned char*>(buf);
        if (bufin == NULL || len == 0 || b64_decode[*bufin] > 63) {
            return TString();
        }
        const unsigned char* bufend = reinterpret_cast<const unsigned char*>(buf) + len;
        while (++bufin < bufend && b64_decode[*bufin] < 64)
            ;
        int nprbytes = static_cast<int>(bufin - reinterpret_cast<const unsigned char*>(buf));
        size_t nbytesdecoded = static_cast<size_t>((nprbytes + 3) / 4) * 3;

        if (nprbytes < static_cast<int>(len)) {
            int left = static_cast<int>(len) - nprbytes;
            while (left--) {
                if (*(bufin++) != '=')
                    return TString();
            }
        }

        TString res;
        res.resize(nbytesdecoded);

        unsigned char* bufout = reinterpret_cast<unsigned char*>(const_cast<char*>(res.data()));
        bufin = reinterpret_cast<const unsigned char*>(buf);

        while (nprbytes > 4) {
            unsigned char a = b64_decode[*bufin];
            unsigned char b = b64_decode[bufin[1]];
            *(bufout++) = static_cast<unsigned char>(a << 2 | b >> 4);
            unsigned char c = b64_decode[bufin[2]];
            *(bufout++) = static_cast<unsigned char>(b << 4 | c >> 2);
            unsigned char d = b64_decode[bufin[3]];
            *(bufout++) = static_cast<unsigned char>(c << 6 | d);
            bufin += 4;
            nprbytes -= 4;
        }

        /* Note: (nprbytes == 1) would be an error, so just ingore that case */
        if (nprbytes > 1) {
            *(bufout++) = static_cast<unsigned char>(b64_decode[*bufin] << 2 | b64_decode[bufin[1]] >> 4);
        }
        if (nprbytes > 2) {
            *(bufout++) = static_cast<unsigned char>(b64_decode[bufin[1]] << 4 | b64_decode[bufin[2]] >> 2);
        }
        if (nprbytes > 3) {
            *(bufout++) = static_cast<unsigned char>(b64_decode[bufin[2]] << 6 | b64_decode[bufin[3]]);
        }

        int diff = (4 - nprbytes) & 3;
        if (diff) {
            nbytesdecoded -= (4 - nprbytes) & 3;
            res.resize(nbytesdecoded);
        }

        return res;
    }

    TString Bin2base64url(const TStringBuf buf, bool pad) {
        return Bin2base64url(buf.data(), buf.size(), pad);
    }

    TString Base64url2bin(const TStringBuf buf) {
        return Base64url2bin(buf.data(), buf.size());
    }

    TString HMAC_sha256(const TStringBuf key, const char* data, size_t dataLen) {
        TString value(EVP_MAX_MD_SIZE, 0);
        unsigned macLen = 0;

        unsigned char* res = ::HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
                                    reinterpret_cast<unsigned char*>(const_cast<char*>(data)),
                                    dataLen, reinterpret_cast<unsigned char*>(const_cast<char*>(value.data())),
                                    &macLen);

        if (!res) {
            return TString();
        }

        if (macLen != EVP_MAX_MD_SIZE) {
            value.resize(macLen);
        }
        return value;
    }

    TString HMAC_sha256(const TStringBuf key, const TStringBuf data) {
        return HMAC_sha256(key, data.data(), data.size());
    }

    TRequestBuilder::TRequestBuilder(const TStringBuf method) {
        Request_.reserve(MAX_REQUEST_LENGTH);
        Request_.assign(method); // don't escape method
    }

    void TRequestBuilder::Add(const TStringBuf key, const TStringBuf val) {
        Request_.append(1, '&').append(URLEncode(key)).append(1, '=').append(URLEncode(val));
    }

    void TRequestBuilder::Add(const TStringBuf key, const TVector<TString>& values) {
        TString val;

        auto p = values.begin();

        if (p != values.end())
            val.append(*p++);

        while (p != values.end()) {
            val.append(1, ',');
            val.append(*p++);
        }

        Add(key, val);
    }

    void TRequestBuilder::Add(const TOption& opt) {
        Add(opt.Key(), opt.Val());
    }

    void TRequestBuilder::Add(const TOptions& options) {
        const TOptions::ListType& list = options.List();

        TOptions::ListType::const_iterator it = list.begin();

        while (it != list.end()) {
            Add(it->Key(), it->Val());
            ++it;
        }
    }

    std::ostream& operator<<(std::ostream& out, TLoginResp::EStatus s) {
        switch (s) {
            case TLoginResp::Valid:
                out << "Valid";
                break;
            case TLoginResp::Disabled:
                out << "Disabled";
                break;
            case TLoginResp::Invalid:
                out << "Invalid";
                break;
            case TLoginResp::ShowCaptcha:
                out << "ShowCaptcha";
                break;
            case TLoginResp::AlienDomain:
                out << "AlienDomain";
                break;
            case TLoginResp::Compromised:
                out << "Compromised";
                break;
            case TLoginResp::Expired:
                out << "Expired";
                break;
            default:
                out << "<UNKNOWN LOGIN STATUS>";
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, TLoginResp::EAccountStatus s) {
        switch (s) {
            case TLoginResp::accUnknown:
                out << "Unknown";
                break;
            case TLoginResp::accValid:
                out << "Valid";
                break;
            case TLoginResp::accAlienDomain:
                out << "AlienDomain";
                break;
            case TLoginResp::accNotFound:
                out << "NotFound";
                break;
            case TLoginResp::accDisabled:
                out << "Disabled";
                break;
            default:
                out << "<UNKNOWN ACCOUNT STATUS>";
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, TLoginResp::EPasswdStatus s) {
        switch (s) {
            case TLoginResp::pwdUnknown:
                out << "Unknown";
                break;
            case TLoginResp::pwdValid:
                out << "Valid";
                break;
            case TLoginResp::pwdBad:
                out << "Bad";
                break;
            case TLoginResp::pwdCompromised:
                out << "Compromised";
                break;
            default:
                out << "<UNKNOWN PASSWORD STATUS>";
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, TSessionResp::EStatus s) {
        switch (s) {
            case TSessionResp::Valid:
                out << "Valid";
                break;
            case TSessionResp::NeedReset:
                out << "NeedReset";
                break;
            case TSessionResp::Expired:
                out << "Expired";
                break;
            case TSessionResp::NoAuth:
                out << "NoAuth";
                break;
            case TSessionResp::Disabled:
                out << "Disabled";
                break;
            case TSessionResp::Invalid:
                out << "Invalid";
                break;
            case TSessionResp::WrongGuard:
                out << "WrongGuard";
                break;
            default:
                out << "<UNKNOWN SESSION STATUS>";
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, TAliasList::TItem::EType t) {
        switch (t) {
            case TAliasList::TItem::Portal:
                out << "Portal";
                break;
            case TAliasList::TItem::Mail:
                out << "Mail";
                break;
            case TAliasList::TItem::NarodMail:
                out << "NarodMail";
                break;
            case TAliasList::TItem::Lite:
                out << "Lite";
                break;
            case TAliasList::TItem::Social:
                out << "Social";
                break;
            case TAliasList::TItem::PDD:
                out << "PDD";
                break;
            case TAliasList::TItem::PDDAlias:
                out << "PDDAlias";
                break;
            case TAliasList::TItem::AltDomain:
                out << "AltDomain";
                break;
            case TAliasList::TItem::Phonish:
                out << "Phonish";
                break;
            case TAliasList::TItem::PhoneNumber:
                out << "PhoneNumber";
                break;
            case TAliasList::TItem::Mailish:
                out << "Mailish";
                break;
            case TAliasList::TItem::Yandexoid:
                out << "Yandexoid";
                break;
            case TAliasList::TItem::KinopoiskId:
                out << "KinopoiskId";
                break;
            default:
                out << static_cast<unsigned int>(t);
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, NSessionCodes::ESessionError e) {
        switch (e) {
            case NSessionCodes::OK:
                out << "OK";
                break;
            case NSessionCodes::UNKNOWN:
                out << "Unknown";
                break;
            case NSessionCodes::INVALID_PARAMS:
                out << "InvalidParams";
                break;
            case NSessionCodes::DB_FETCHFAILED:
                out << "DBFetchFailed";
                break;
            case NSessionCodes::DB_EXCEPTION:
                out << "DBException";
                break;
            case NSessionCodes::ACCESS_DENIED:
                out << "AccessDenied";
                break;
            default:
                out << static_cast<unsigned int>(e);
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, TBruteforcePolicy::EMode m) {
        switch (m) {
            case TBruteforcePolicy::None:
                out << "None";
                break;
            case TBruteforcePolicy::Captcha:
                out << "Captcha";
                break;
            case TBruteforcePolicy::DelayMode:
                out << "Delay";
                break;
            case TBruteforcePolicy::Expired:
                out << "Expired";
                break;
            default:
                out << "<Unknown>";
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, TSessionKind::EKind k) {
        switch (k) {
            case TSessionKind::None:
                out << "None";
                break;
            case TSessionKind::Support:
                out << "Support";
                break;
            case TSessionKind::Stress:
                out << "Stress";
                break;
            default:
                out << static_cast<unsigned>(k);
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TEmailList::TItem& item) {
        out << "addr='" << item.Address() << "', born date='" << item.Date() << "'";

        if (item.IsDefault())
            out << ", default";

        if (item.Native())
            out << ", native";
        else if (item.Rpop())
            out << ", remote pop";
        else
            out << (item.Validated() ? "," : ", not") << " validated";

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TAliasList::TItem& item) {
        out << "type='" << item.type() << "', alias='" << item.alias() << "'";

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TPDDUserInfo& info) {
        out << "PDD user info: domainId=" << info.DomId()
            << ", domain='" << info.Domain()
            << "', mx='" << info.Mx()
            << "', domEna='" << info.DomEna()
            << "', catchAll='" << info.CatchAll() << "'";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TKarmaInfo& info) {
        out << "Karma info: karma=" << info.Karma() << ", banTime='" << info.BanTime()
            << "', karmaStatus='" << info.KarmaStatus() << "'";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TDisplayNameInfo& info) {
        out << "DisplayName: '" << info.Name() << "'";
        if (info.Social())
            out << ", social account, provider='" << info.SocProvider()
                << "', profileId=" << info.SocProfile()
                << ", redirectTarget=" << info.SocTarget();
        if (!info.DefaultAvatar().empty())
            out << std::endl
                << "Default avatar: " << info.DefaultAvatar();
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TAuthInfo& info) {
        out << "Password verified " << info.Age() << " sec ago";
        if (info.Social())
            out << ", social authorization, profile id=" << info.ProfileId();
        out << ", session " << (info.Secure() ? "secure" : "insecure");
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TEmailList& data) {
        if (data.Empty())
            return out; // no emails, no default

        const TEmailList::ListType& mlist = data.GetEmailItems();

        TEmailList::ListType::const_iterator p = mlist.begin();

        out << "Emails list: (size=" << mlist.size() << ") [" << std::endl;
        ;
        while (p != mlist.end())
            out << "\t" << *p++ << std::endl;
        out << "]" << std::endl;

        if (!data.GetDefault().empty())
            out << "Default email: '" << data.GetDefault() << "'" << std::endl;

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TAliasList& data) {
        if (data.Empty())
            return out;

        const TAliasList::TListType& alist = data.GetAliases();
        TAliasList::TListType::const_iterator p = alist.begin();

        out << "Alias list: (size=" << alist.size() << ") [" << std::endl;
        ;
        while (p != alist.end())
            out << "\t" << *p++ << std::endl;
        out << "]" << std::endl;

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TDBFields& data) {
        if (data.Empty())
            return out;

        auto p = data.Begin();

        out << "DBFields: (size=" << data.Size() << ") [" << std::endl;
        while (p != data.End()) {
            out << "\t'" << p->first << "'='" << p->second << "'" << std::endl;
            ++p;
        }
        out << "]" << std::endl;

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TAttributes& data) {
        if (data.Empty())
            return out;

        auto p = data.Begin();

        out << "Attributes: (size=" << data.Size() << ") [" << std::endl;
        while (p != data.End()) {
            out << "\t'" << p->first << "'='" << p->second << "'" << std::endl;
            ++p;
        }
        out << "]" << std::endl;

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TOAuthInfo& data) {
        if (data.Empty())
            return out;

        const TOAuthInfo::TMapType& info = data.GetInfo();
        TOAuthInfo::TMapType::const_iterator p = info.begin();

        out << "OAuth info: (size=" << info.size() << ") [ ";
        out << "'" << p->first << "':'" << p->second << "'";
        ++p;
        while (p != info.end()) {
            out << ", "
                << "'" << p->first << "':'" << p->second << "'";
            ++p;
        }
        out << " ]" << std::endl;

        return out;
    }

    template <class R>
    static void PrintResponseData(std::ostream& out, const R* resp) {
        out << "Response message: '" << resp->Message() << "'" << std::endl;

        // query all accessors we know
        TRegname regname(resp);
        TUid uid(resp);
        TLiteUid liteUid(resp);
        TPDDUserInfo pddInfo(resp);
        TKarmaInfo karma(resp);
        TDisplayNameInfo dispName(resp);
        TLoginInfo login(resp);

        out << "User info: ";

        if (!regname.Value().empty())
            out << "regname='" << regname.Value() << "', ";

        if (!uid.Uid().empty())
            out << "uid=" << uid.Uid();

        if (!liteUid.LiteUid().empty())
            out << "liteUid=" << liteUid.LiteUid();
        out << std::endl;

        if (uid.Hosted())
            out << pddInfo << std::endl;

        out << karma << std::endl;
        out << "Login name: '" << login.Login() << "', havePassword="
            << (login.HavePassword() ? "true" : "false") << ", haveHint="
            << (login.HaveHint() ? "true" : "false") << std::endl;

        if (!dispName.Name().empty())
            out << dispName << std::endl;

        TEmailList emails(resp);
        TAliasList aliases(resp);
        TDBFields fields(resp);
        TAttributes attrs(resp);

        out << emails;
        out << aliases;
        out << fields;
        out << attrs;
    }

    std::ostream& operator<<(std::ostream& out, const TResponse* resp) {
        if (!resp) {
            out << "NULL Response." << std::endl;
            return out;
        }

        PrintResponseData(out, resp);

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TBulkResponse* pBulkResp) {
        if (!pBulkResp) {
            out << "NULL BulkResponse." << std::endl;
            return out;
        }

        out << "Bulk Response with " << pBulkResp->Count() << " children"
            << " and message '" << pBulkResp->Message() << "'" << std::endl
            << std::endl;

        for (int i = 0; i < pBulkResp->Count(); ++i) {
            out << "Bulk child #" << i << " with id='" << pBulkResp->Id(i) << "':" << std::endl;
            out << pBulkResp->User(i).Get() << std::endl;
        }

        PrintResponseData(out, pBulkResp);

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TLoginResp* pLogResp) {
        if (!pLogResp) {
            out << "NULL LoginResponse." << std::endl;
            return out;
        }

        out << "Login Response with status: " << pLogResp->Status() << std::endl;
        out << "Account status: " << pLogResp->AccStatus()
            << ", password status: " << pLogResp->PwdStatus() << std::endl;

        TBruteforcePolicy policy(pLogResp);

        if (policy.Mode() != TBruteforcePolicy::None) {
            out << "Bruteforce policy: mode=" << policy.Mode();
            if (policy.Mode() == TBruteforcePolicy::DelayMode)
                out << ", delay=" << policy.Delay();
            out << std::endl;
        }

        TConnectionId connectionId(pLogResp);

        if (!connectionId.ConnectionId().empty()) {
            out << "Connection Id: " << connectionId.ConnectionId() << std::endl;
        }

        PrintResponseData(out, pLogResp);

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TSessionResp* pSessResp) {
        if (!pSessResp) {
            out << "NULL SessionResponse." << std::endl;
            return out;
        }

        out << "Session Response with status: " << pSessResp->Status() << std::endl;

        out << "Session type: " << (pSessResp->IsLite() ? "Lite" : "Full");
        out << ", age: " << pSessResp->Age() << std::endl;

        TNewSessionId newId(pSessResp);

        if (!newId.Value().empty())
            out << "New sessionId='" << newId.Value() << "', domain='" << newId.Domain()
                << "', expires='" << newId.Expires() << "', httpOnly="
                << (newId.HttpOnly() ? "true" : "false") << std::endl;

        PrintResponseData(out, pSessResp);

        TAuthInfo authInfo(pSessResp);
        TOAuthInfo oauthInfo(pSessResp);
        TSessionKind kind(pSessResp);
        TAuthId authid(pSessResp);

        if (!authInfo.Age().empty())
            out << authInfo << std::endl;

        if (!oauthInfo.Empty())
            out << oauthInfo;

        if (kind.Kind() != TSessionKind::None)
            out << "Session kind: " << kind.Kind() << " ( '" << kind.KindName() << "' )" << std::endl;

        if (!authid.AuthId().empty())
            out << "AuthId: '" << authid.AuthId() << "', time=" << authid.Time()
                << ", ip=" << authid.UserIp() << ", host=" << authid.HostId() << std::endl;

        TConnectionId connectionId(pSessResp);

        if (!connectionId.ConnectionId().empty()) {
            out << "Connection Id: " << connectionId.ConnectionId() << std::endl;
        }

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const TMultiSessionResp* pMultiSessResp) {
        if (!pMultiSessResp) {
            out << "NULL MultiSessionResponse." << std::endl;
            return out;
        }

        out << "MultiSession Response with " << pMultiSessResp->Count() << " children and status: " << pMultiSessResp->Status() << std::endl;

        out << "Session age: " << pMultiSessResp->Age() << std::endl;
        out << "Response message: '" << pMultiSessResp->Message() << "'" << std::endl;
        out << "Default uid: " << pMultiSessResp->DefaultUid() << std::endl;

        TNewSessionId newId(pMultiSessResp);

        if (!newId.Value().empty())
            out << "New sessionId='" << newId.Value() << "', domain='" << newId.Domain()
                << "', expires='" << newId.Expires() << "', httpOnly="
                << (newId.HttpOnly() ? "true" : "false") << std::endl;

        for (int i = 0; i < pMultiSessResp->Count(); ++i) {
            out << "Multisession child #" << i << " with id='" << pMultiSessResp->Id(i) << "':" << std::endl;
            out << pMultiSessResp->User(i).Get() << std::endl;
        }

        TSessionKind kind(pMultiSessResp);
        TAuthId authid(pMultiSessResp);

        if (kind.Kind() != TSessionKind::None)
            out << "Session kind: " << kind.Kind() << " ( '" << kind.KindName() << "' )" << std::endl;

        if (!authid.AuthId().empty())
            out << "AuthId: '" << authid.AuthId() << "', time=" << authid.Time()
                << ", ip=" << authid.UserIp() << ", host=" << authid.HostId() << std::endl;

        TAllowMoreUsers allow_more(pMultiSessResp);

        out << "Allow more users: " << (allow_more.AllowMoreUsers() ? "true" : "false") << std::endl;

        TConnectionId connectionId(pMultiSessResp);

        if (!connectionId.ConnectionId().empty()) {
            out << "Connection Id: " << connectionId.ConnectionId() << std::endl;
        }

        return out;
    }

    /// Static Curl library initializer
    //CUrl CUrl::instance_;

}

namespace NIdn {
    // returns true if result is in converted, false - str does not need conversion
    static bool IdnaEncode(TString::const_iterator strBeg, TString::const_iterator strEnd, TString& converted) {
        // First, make sure originalTString does contain non-ASCII chars; empty
        //TStrings fall into this category as well
        TString::const_iterator it;
        for (it = strBeg; it != strEnd; ++it)
            if (static_cast<unsigned char>(*it) > 127)
                break;
        if (it == strEnd)
            return false;

        // Yes, need to convert. Split into DNS parts and conver them independently
        // because each part is limited in length: it may not exceed 63 characters
        converted.clear();
        converted.reserve((strEnd - strBeg) * 3);

        bool firstRun = true;
        TString part;
        TString::const_iterator rwall = strBeg;
        for (;;) {
            // End ofTString
            if (rwall == strEnd)
                break;

            // Move onto the next part, which must begin with a dot in all
            // cases except for the very first one
            if (!firstRun)
                converted.push_back('.');

            // Next part turns out to be just the dot  and nothing more
            if (++rwall == strEnd)
                break;

            // Find right wall for the current part
            TString::const_iterator lwall = firstRun ? strBeg : rwall;
            rwall = std::find(lwall, strEnd, '.');

            // If it's empty simply skip over
            if (rwall != lwall) {
                part.assign(lwall, rwall);

                char* tmp = NULL;

                // Try to convert theTString
                Idna_rc idnaRet = static_cast<Idna_rc>(idna_to_ascii_8z(part.c_str(), &tmp, 0));
                if (idnaRet != IDNA_SUCCESS) {
                    if (tmp)
                        free(tmp);
                    throw std::runtime_error(TString("IDNA error: '") + idna_strerror(idnaRet) + "', str=" + TString(strBeg, strEnd));
                }

                converted.append(tmp);
                free(tmp);
            }

            firstRun = false;
        }

        // Further "normalize" theTString by lowering its case
        to_lower(converted);

        return true;
    }

    // returns true if result is in converted, false - str does not need conversion
    static bool IdnaDecode(TString::const_iterator strBeg, TString::const_iterator strEnd, TString& converted) {
        const TString prefix("xn--");

        // check if need conversion
        if (std::search(strBeg, strEnd, prefix.begin(), prefix.end()) == strEnd)
            return false;

        // Split into DNS parts and conver them independently
        // because each part is limited in length: it may not exceed 63 characters
        converted.clear();
        converted.reserve((strEnd - strBeg) * 3);

        bool firstRun = true;
        TString part;
        TString::const_iterator rwall = strBeg;
        for (;;) {
            // End ofTString
            if (rwall == strEnd)
                break;

            // Move onto the next part, which must begin with a dot in all
            // cases except for the very first one
            if (!firstRun)
                converted.push_back('.');

            // Next part turns out to be just the dot  and nothing more
            if (++rwall == strEnd)
                break;

            // Find right wall for the current part
            TString::const_iterator lwall = firstRun ? strBeg : rwall;
            rwall = std::find(lwall, strEnd, '.');

            // If it's empty simply skip over
            if (rwall != lwall) {
                part.assign(lwall, rwall);

                char* tmp = NULL;

                // Try to convert theTString
                Idna_rc idnaRet = static_cast<Idna_rc>(idna_to_unicode_8z8z(part.c_str(), &tmp, 0));
                if (idnaRet != IDNA_SUCCESS) {
                    if (tmp)
                        free(tmp);
                    throw std::runtime_error(TString("IDNA error: '") + idna_strerror(idnaRet) + "', str=" + TString(strBeg, strEnd));
                }

                converted.append(tmp);
                free(tmp);
            }

            firstRun = false;
        }

        return true;
    }

    // encode entireTString to punycode
    const TString& Encode(const TString& str, TString& converted) {
        if (IdnaEncode(str.begin(), str.end(), converted))
            return converted;
        else
            return str; // no change
    }

    // decode entireTString as punycode
    const TString& Decode(const TString& str, TString& converted) {
        if (IdnaDecode(str.begin(), str.end(), converted))
            return converted;
        else
            return str;
    }

    // encode address to punycode
    const TString& EncodeAddr(const TString& addr, TString& converted) {
        TString::const_iterator it = std::find(addr.begin(), addr.end(), '@');
        if (it == addr.end() || ++it == addr.end())
            return addr; // no @ or ends with @

        TString tmp;
        if (IdnaEncode(it, addr.end(), tmp)) {
            converted.clear();
            converted.reserve(tmp.size() + (it - addr.begin()) + 1);
            converted.assign(addr.begin(), it);
            converted.append(tmp);
            return converted;
        } else
            return addr; // no conversion needed
    }

    // decode address as punycode
    const TString& DecodeAddr(const TString& addr, TString& converted) {
        TString::const_iterator it = std::find(addr.begin(), addr.end(), '@');
        if (it == addr.end() || ++it == addr.end())
            return addr; // no @ or ends with @

        TString tmp;
        if (IdnaDecode(it, addr.end(), tmp)) {
            converted.clear();
            converted.reserve(tmp.size() + (it - addr.begin()) + 1);
            converted.assign(addr.begin(), it);
            converted.append(tmp);
            return converted;
        } else
            return addr; // no conversion needed
    }

}

// vi: expandtab:sw=4:ts=4
// kate: replace-tabs on; indent-width 4; tab-width 4;
