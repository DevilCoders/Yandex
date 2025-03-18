#include "fullreq_info.h"

#include "antirobot_experiment.h"
#include "autoru_tamper.h"
#include "config_global.h"
#include "host_ops.h"
#include "request_classifier.h"
#include "return_path.h"

#include <antirobot/idl/cache_sync.pb.h>
#include <antirobot/lib/fuid.h>
#include <antirobot/lib/http_helpers.h>
#include <antirobot/lib/ip_list.h>
#include <antirobot/lib/log_utils.h>
#include <antirobot/lib/spravka.h>
#include <antirobot/lib/yandexuid.h>
#include <antirobot/lib/yandex_trust_keyring.h>

#include <yweb/webdaemons/icookiedaemon/icookie_lib/process.h>

#include <contrib/libs/jwt-cpp/include/jwt-cpp/jwt.h>

#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/regex/pire/pire.h>
#include <library/cpp/regex/regexp_classifier/regexp_classifier.h>
#include <library/cpp/string_utils/scan/scan.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/uilangdetect/byacceptlang.h>
#include <library/cpp/uilangdetect/bycookie.h>

#include <util/datetime/base.h>
#include <util/digest/city.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/type.h>
#include <util/string/vector.h>

#include <utility>

namespace NAntiRobot {
    namespace {
        constexpr TStringBuf DISABLE_YANDEX = "QUxD7VxS8MA9voVwZdHsjz9wFj80iVcZ"_sb;
        constexpr TStringBuf DISABLE_PRIVILEGED = "4tMDnU4MRQyvo3SyEKkq89wcAAA6ZJnq"_sb;
        constexpr TStringBuf DISABLE_WHITELIST = "gBQeYpMlWFqFExSpsuB8j89vDDlf7HaT"_sb;

        constexpr TStringBuf YANDEX_JWS_HEADER = "X-Yandex-Jws"_sb;

        static TSet<TString> COMMON_TLD = {
                "biz",
                "com",
                "edu",
                "info",
                "int",
                "mobi",
                "net",
                "org",
        };

        TString MaskCookieSignature(TStringBuf cookieString) {
            size_t offsetToMask = ANTIROBOT_DAEMON_CONFIG.CharactersFromPrivateCookieToHide;
            static const TString maskCharacters(offsetToMask, '*');
            TStringBuf unmaskedPart = cookieString.Chop(offsetToMask);
            TStringBuf mask = TStringBuf(maskCharacters).Head(offsetToMask);
            return TString::Join(unmaskedPart, mask);
        }

        struct TCookiesAppender {
            TString* cookiesString;

            inline void operator()(TStringBuf key, TStringBuf value) {
                if (!cookiesString->Empty()) {
                    cookiesString->append("; ");
                }
                cookiesString->append(StripString(key));
                cookiesString->append("=");
                if (ANTIROBOT_DAEMON_CONFIG.SuppressedCookiesInLogsSet.contains(StripString(key))) {
                    cookiesString->append(StripString(MaskCookieSignature(value)));
                } else {
                    cookiesString->append(StripString(value));
                }
            }
        };

        bool IsPartner(const TCgiParameters& cgiParams) {
            return cgiParams.Has("showmecaptcha"_sb, "yes"_sb);
        }

        inline TStringBuf SkipDomain(TStringBuf rs) {
            if (rs.empty() || StartsWith(rs, "/"_sb))
                return rs;

            TStringBuf rsWithoutScheme = CutSchemePrefix(rs);
            if (rsWithoutScheme.size() == rs.size())
                return rs;

            TStringBuf domain;
            TStringBuf req;

            if (rsWithoutScheme.TrySplit('/', domain, req))
               return TStringBuf(req.data() - 1, req.size() + 1);

            return "/"_sb;
        }

        void CheckForPartnerRequest(TFullReqInfo& fr) {
            if (EqualToOneOf(fr.Doc, "/xmlsearch"_sb, "/search/xml"_sb, "/images-xml"_sb) && IsPartner(fr.CgiParams) ||
                fr.Doc == "/xcheckcaptcha"_sb && ANTIROBOT_DAEMON_CONFIG.ConfByTld(fr.Tld).PartnerCaptchaType) {
                if (!fr.Headers.Has("X-Real-Ip"_sb))
                    ythrow TFullReqInfo::TInvalidPartnerRequest();

                fr.PartnerAddr = fr.RawAddr;
            }
        }

        bool NotEmpty(const TStringBuf& s) {
            return !s.empty();
        }

        TStringBuf GetHost(const THeadersMap& headers) {
            const TStringBuf values[] = {
                headers.Get("X-Host-Y"_sb),
                headers.Get("Host"_sb),
                ANTIROBOT_DAEMON_CONFIG.DefaultHost,
            };

            return *std::find_if(values, values + Y_ARRAY_SIZE(values), NotEmpty);
        }

        bool SpravkaExpired(const TSpravka* s, TInstant now) noexcept {
            return now > s->Time + ANTIROBOT_DAEMON_CONFIG.SpravkaExpireInterval;
        }

        TStringBuf CalcServiceReqid(const EHostType hostType, TStringBuf baseReqid, const THeadersMap& headers) {
            if (IsIn(VERTICAL_TYPES, hostType)) {
                TStringBuf autoruReqid = headers.Get("X-Request-Id"_sb);
                if (!autoruReqid.empty()) {
                    return autoruReqid;
                }
            } else if (IsIn(MARKET_TYPES_EXT, hostType)) {
                TStringBuf marketReqid = headers.Get("X-Market-Req-Id"_sb);
                if (!marketReqid.empty()) {
                    return marketReqid;
                }
            }

            return baseReqid;
        }

        bool IsCaptchaUrl(const TStringBuf location) {
            return
                location.StartsWith("/showcaptcha") ||
                location.StartsWith("/checkcaptcha") ||
                location.StartsWith("/xcheckcaptcha");
        }

        TMaybe<EHostType> GetHostType(const THttpInfo& httpReq) {
            if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
                return HOST_OTHER;
            }

            static const TStringBuf HOST_HEADER("X-Antirobot-Service-Y");
            const TStringBuf serviceHeaderVal = httpReq.Headers.Get(HOST_HEADER);
            if (serviceHeaderVal.empty()) {
                const auto& cgiParams = httpReq.CgiParams;
                if (!IsCaptchaUrl(httpReq.Doc) || cgiParams.Get(TReturnPath::CGI_PARAM_NAME).empty()) {
                    return HostToHostType(httpReq.Host, httpReq.Doc, httpReq.Cgi);
                }
                try {
                    const TString retPathUrl = TReturnPath::FromCgi(cgiParams).GetURL();
                    return HostToHostType(retPathUrl);
                } catch (const TReturnPath::TInvalidRetPathException& ex) {
                    return Nothing();
                }
            } else {
                EHostType hostType;
                if (TryFromString(serviceHeaderVal, hostType)) {
                    return hostType;
                }
                return Nothing();
            }
        }

        ECaptchaReqType GetCaptchaReqType(const TStringBuf& doc) {
            static const std::pair<TStringBuf, ECaptchaReqType> mapping[] = {
                {"/checkcaptcha"_sb,     CAPTCHAREQ_CHECK},
                {"/tmgrdfrendc"_sb,        CAPTCHAREQ_CHECK},
                {"/xcheckcaptcha"_sb,    CAPTCHAREQ_CHECK},
                {"/checkcaptchajson"_sb, CAPTCHAREQ_CHECK},
                {"/showcaptcha"_sb,      CAPTCHAREQ_SHOW},
                {"/captchaimg"_sb,       CAPTCHAREQ_IMAGE},
            };
            static const THashMap<TStringBuf, ECaptchaReqType> doc2captchaType(mapping, mapping + Y_ARRAY_SIZE(mapping));

            if (const ECaptchaReqType* ptr = doc2captchaType.FindPtr(doc)) {
                return *ptr;
            } else {
                return CAPTCHAREQ_NONE;
            }
        }

        EClientType GetClientType(const THttpInfo& req, EHostType hostType, EReqType reqType,
                                  const TStringBuf& yandexUid)
        {
            auto cgi = [&](const TStringBuf& param) {
                return req.CgiParams.Get(param);
            };

            if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
                return CLIENT_CAPTCHA_API;
            }

            if (hostType == HOST_IMAGES && reqType == REQ_YANDSEARCH
                && EqualToOneOf(cgi("rpt"_sb), "imageajax"_sb, "imagedupsajax"_sb))
            {
                return CLIENT_AJAX;
            }

            if (hostType == HOST_USLUGI && reqType == REQ_YANDSEARCH
                    && (req.Doc.StartsWith("/uslugi/api/"_sb) || req.Doc.StartsWith("/api/"_sb)))
            {
                return CLIENT_AJAX;
            }

            if (hostType == HOST_INVEST && req.Doc.StartsWith("/graphql"_sb))
            {
                return CLIENT_AJAX;
            }

            if (reqType == REQ_MSEARCH && !yandexUid.empty() && !cgi("callback"_sb).empty()
                && yandexUid == cgi("yu"_sb))
            {
                return CLIENT_AJAX;
            }

            if (reqType == REQ_SITESEARCH && IsTrue(cgi("html"_sb))) {
                return CLIENT_AJAX;
            }

            if (req.Doc == "/xcheckcaptcha"_sb) {
                return CLIENT_XML_PARTNER;
            } else if (req.Doc == "/checkcaptchajson"_sb) {
                return CLIENT_AJAX;
            } else if (hostType == HOST_SLOVARI && StartsWith(req.Doc, "/~p/"_sb)) {
                return CLIENT_AJAX;
            }

            if (req.Headers.Get("X-Requested-With"_sb) == "XMLHttpRequest"_sb
                || IsTrue(cgi("ajax"_sb))
                || cgi("format"_sb) == "json"_sb)
            {
                return CLIENT_AJAX;
            }

            return CLIENT_GENERAL;
        }

        EReqType CalcReqType(const THttpInfo& req, EHostType hostType, const TReloadableData& reloadableData) {
            if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
                return REQ_OTHER;
            }

            static const TStringBuf MAYBANFOR_HEADER("X-Antirobot-MayBanFor-Y");

            const TStringBuf& mayBanForValue = req.Headers.Get(MAYBANFOR_HEADER);
            if (!mayBanForValue.empty()) {
                return IsTrue(mayBanForValue) ? REQ_YANDSEARCH : REQ_OTHER;
            }

            if (req.IsNewsClickReq()) {
                return REQ_NEWS_CLICK;
            }

            const auto& cgi = req.CgiParams;
            const auto& classifier = reloadableData.RequestClassifier.Get();
            const EReqType result = classifier->DetectRequestType(hostType, req.Doc, req.Cgi, REQ_OTHER);

            int cgiWeb = 0;
            TryFromString(cgi.Get("web"_sb), cgiWeb);
            if (result == REQ_SITESEARCH && cgiWeb == 1) {
                return REQ_YANDSEARCH;
            }

            if (hostType == HOST_BLOGS && result == REQ_YANDSEARCH
                && !cgi.Has("text"_sb) && !cgi.Get("id"_sb).empty()
                && cgi.Has("cat"_sb, "theme"_sb))
            {
                return REQ_THEME;
            }

            if (result == REQ_SITESEARCH && cgi.Find("text"_sb) == cgi.end()) {
                return REQ_OTHER;
            }

            return result;
        }

        void ProcessAntirobotCookie(const TEnv& env, TRequest* req) {
            TAntirobotCookie arCookie;

            const TStringBuf arCookieStr = req->Cookies.Get(ANTIROBOT_COOKIE_KEY);

            try {
                arCookie = TAntirobotCookie::Decrypt(env.YascKey, env.LastVisitsIds, arCookieStr);
            } catch (...) {}

            TVector<TLastVisitsCookie::TRuleId> detectedIds;

            for (const auto& ruleKey : env.LastVisitsRuleSet.Match(*req)) {
                detectedIds.push_back(static_cast<TLastVisitsCookie::TRuleId>(ruleKey.Key.Group));
            }

            req->AntirobotCookieDirty =
                arCookie.LastVisitsCookie.Touch(detectedIds) ||
                arCookie.LastVisitsCookie.IsStale();

            req->AntirobotCookie = std::move(arCookie);
        }

        bool GetBooleanJwsClaim(const jwt::decoded_jwt& token, const std::string& key) {
            const auto& claim = token.get_payload_claim(key);
            return claim.get_type() == jwt::claim::type::boolean && claim.as_bool();
        }

        bool IsSuspiciousMarketJws(const jwt::decoded_jwt& token) {
            // Absence of basicIntegrity/ctsProfileMatch indicates that this market token is from
            // iOS and therefore assumed to be unsuspicious.
            return
                (
                    token.has_payload_claim("basicIntegrity") &&
                    token.has_payload_claim("ctsProfileMatch")
                ) &&
                (
                    !GetBooleanJwsClaim(token, "basicIntegrity") ||
                    !GetBooleanJwsClaim(token, "ctsProfileMatch")
                );
        }

        bool IsSuspiciousJws(const jwt::decoded_jwt& token) {
            // device_integrity should always be present in tokens from narwhal.
            return
                !token.has_payload_claim("device_integrity") ||
                !GetBooleanJwsClaim(token, "device_integrity");
        }

        bool IsValidJws(const jwt::decoded_jwt& token, const std::string& key) {
            try {
                const auto data = token.get_header_base64() + "." + token.get_payload_base64();
                const auto& sig = token.get_signature();
                jwt::algorithm::hs256(key).verify(data, sig);
            } catch (...) {
                return false;
            }

            return true;
        }

        bool IsExpiredJws(
            const jwt::decoded_jwt& token,
            const std::string& claimKey,
            TInstant (*instantCtor)(ui64),
            TDuration leeway
        ) {
            if (!token.has_payload_claim(claimKey)) {
                return true;
            }

            const auto& claim = token.get_payload_claim(claimKey);
            if (claim.get_type() != jwt::claim::type::int64) {
                return true;
            }

            const i64 claimValue = claim.as_int();
            return claimValue < 0 || TInstant::Now() >= instantCtor(claimValue) + leeway;
        }

        std::pair<EJwsPlatform, EJwsState> CheckMarketJws(
            const std::string& s, const std::string& key
        ) {
            if (s.empty()) {
                return {EJwsPlatform::Unknown, EJwsState::Invalid};
            }

            try {
                jwt::decoded_jwt token(s);

                if (!IsValidJws(token, key)) {
                    return {EJwsPlatform::Unknown, EJwsState::Invalid};
                }

                const auto platform = token.has_payload_claim("basicIntegrity") ?
                    EJwsPlatform::Android : EJwsPlatform::Ios;

                const auto isExpired = IsExpiredJws(
                    token, "exp", TInstant::Seconds,
                    ANTIROBOT_DAEMON_CONFIG.MarketJwsLeeway
                );
                const auto state = IsSuspiciousMarketJws(token) ?
                    (isExpired ? EJwsState::SuspExpired : EJwsState::Susp) :
                    (isExpired ? EJwsState::ValidExpired : EJwsState::Valid);

                return {platform, state};
            } catch (...) {
                return {EJwsPlatform::Unknown, EJwsState::Invalid};
            }
        }

        std::pair<EJwsPlatform, EJwsState> CheckNarwhalJws(
            const std::string& s,
            const TEnv& env
        ) {
            if (s.empty()) {
                return {EJwsPlatform::Unknown, EJwsState::Invalid};
            }

            try {
                jwt::decoded_jwt token(s);

                const TString* key = nullptr;
                bool isDefault = false;

                const auto& keyId = token.get_key_id();
                if (keyId == env.NarwhalJwsKeyId) {
                    key = &env.NarwhalJwsKey;
                } else if (!env.BalancerJwsKeyId.empty() && keyId == env.BalancerJwsKeyId) {
                    key = &env.BalancerJwsKey;
                    isDefault = true;
                } else {
                    return {EJwsPlatform::Unknown, EJwsState::Invalid};
                }

                if (!IsValidJws(token, *key)) {
                    return {EJwsPlatform::Unknown, EJwsState::Invalid};
                }

                const auto platform = token.has_payload_claim("android_claims") ?
                    EJwsPlatform::Android : EJwsPlatform::Ios;

                const auto isExpired = IsExpiredJws(
                    token, "expires_at_ms", TInstant::MilliSeconds,
                    ANTIROBOT_DAEMON_CONFIG.NarwhalJwsLeeway
                );

                EJwsState state;

                if (isDefault) {
                    state = isExpired ? EJwsState::DefaultExpired : EJwsState::Default;
                } else if (IsSuspiciousJws(token)) {
                    state = isExpired ? EJwsState::SuspExpired : EJwsState::Susp;
                } else {
                    state = isExpired ? EJwsState::ValidExpired : EJwsState::Valid;
                }

                return {platform, state};
            } catch (...) {
                return {EJwsPlatform::Unknown, EJwsState::Invalid};
            }
        }

        TInstant LdapTimestampToInstant(ui64 ldapTimestamp) {
            const ui64 delta = 11644473600;
            ldapTimestamp /= 1000000;
            ui64 seconds = ldapTimestamp <= delta ? 0 : ldapTimestamp - delta;
            return TInstant::Seconds(seconds);
        }

        struct TYandexTrustTokenScanner {
            TStringBuf Time;
            TStringBuf Digest;
            TStringBuf DigestKey;

            inline bool Valid() const noexcept {
                return !!Time && !!Digest;
            }

            inline void operator() (TStringBuf key, TStringBuf value) {
                if (key == "time"sv) {
                    Time = value;
                } else if (key == "digest"sv) {
                    DigestKey = key;
                    Digest = value;
                }
            }
        };

        EYandexTrustState GetYandexTrustState(const TRequest& req) {
            TStringBuf headerValue = req.Headers.Get("Yandex-Trust");
            if (!headerValue) {
                return EYandexTrustState::Invalid;
            }

            TYandexTrustTokenScanner scanner;
            ScanKeyValue<false, ';', '='>(headerValue, scanner);
            if (!scanner.Valid()) {
                return EYandexTrustState::Invalid;
            }

            TStringBuf data(headerValue.begin(), scanner.DigestKey.begin());

            if (!TYandexTrustKeyRing::Instance()->IsSignedHex(data, scanner.Digest)) {
                return EYandexTrustState::Invalid;
            }

            ui64 ldapTimestamp;
            if (!TryFromString<ui64>(scanner.Time, ldapTimestamp)) {
                return EYandexTrustState::ValidExpired;
            }
            auto time = LdapTimestampToInstant(ldapTimestamp);
            if (time + ANTIROBOT_DAEMON_CONFIG.YandexTrustTokenExpireInterval < req.ArrivalTime) {
                return EYandexTrustState::ValidExpired;
            }

            return EYandexTrustState::Valid;
        }

        class THeaderHashes {
        private:
            Y_DECLARE_SINGLETON_FRIEND();
            THeaderHashes();
        public:
            THashMap<TString, TString> HeaderToHash;
            THashMap<TString, TString> HashToHeader;
        };

        THeaderHashes::THeaderHashes() {
            /*
            Для каждого(218) заголовка из списка https://yt.yandex-team.ru/hahn/navigation?path=//home/antirobot/users/gasafyanov/important_headers_sensitive
            было присвоено уникальное значение из алфавита [a-Z] в возрастающем порядке: aa, ab, ..., az, aA, ...
            */
            TString configString;
            Y_ENSURE(NResource::FindExact("header_hashes.json", &configString));
            NJson::TJsonValue json;
            Y_ENSURE(NJson::ReadJsonTree(configString, &json));
            for (const auto& [header, hash] : json.GetMap()) {
                HeaderToHash[header] = hash.GetString();
                HashToHeader[hash.GetString()] = header;
            }
            Y_ENSURE(HashToHeader.size() == HeaderToHash.size());
        }

        auto loader = Singleton<THeaderHashes>();

    } // anonymous namespace

    THttpInfo::THttpInfo(THttpInput& input, const TString& requesterAddr, TTimeStats* readStats,
                         bool failOnReadRequestTimeout, bool isWrappedRequest)
        : FirstLine(input.FirstLine())
        , HttpHeaders(input.Headers())
    {
        RequesterAddr = requesterAddr;

        try {
            TParsedHttpRequest parsedReq(FirstLine);

            RequestMethod = parsedReq.Method;
            RequestString = parsedReq.Request;
            RequestProtocol = parsedReq.Proto;
        } catch(...) {
            ythrow TBadRequest() << "Error while parsing method-request-protocol, line: " << FirstLine;
        }

        if ((RequestMethod.size() + RequestProtocol.size()) > sizeof(Buf_)) {
            ythrow TBadRequest() << "Method and size are too long, line: " << FirstLine;
        }

        RequestMethod = ToLower(RequestMethod, Buf_);
        RequestProtocol = ToLower(RequestProtocol, (char*)RequestMethod.end());

        SplitUri(RequestString, Doc, Cgi);
        CgiParams.Scan(Cgi);

        for (THttpHeaders::TConstIterator toHeader = HttpHeaders.Begin(); toHeader != HttpHeaders.End(); ++toHeader) {
            Headers.Add(toHeader->Name(), toHeader->Value());
            CSHeaders.emplace(toHeader->Name(), toHeader->Value());
            if (auto it = loader->HeaderToHash.find(toHeader->Name()); it != loader->HeaderToHash.end()) {
                Hodor += it->second + "-";
            }
            HodorHash += toHeader->Name();

            if (EHeaderOrderNames headerPos; TryFromString<EHeaderOrderNames>(toHeader->Name(), headerPos)) {
                HeaderPos[static_cast<size_t>(headerPos)] = std::min(toHeader - HttpHeaders.Begin() + 1, 255L);
            }
        }

        Uuid = Headers.Get("X-Device-UUID"_sb);
        if (Uuid.empty()) {
            Uuid = Headers.Get("UUID"_sb);
            if (Uuid.empty()) {
                Uuid = CgiParams.Get("uuid"_sb);
            }
        }

        Cookies.Scan(CookiesString());

        ICookie = Headers.Get(NIcookie::ICOOKIE_DECRYPTED_HEADER);
        if (ICookie.empty()) {
            if (const auto encryptedIcookie = Cookies.Get("i");
                !encryptedIcookie.empty()) {
                if (const auto maybeIcookie = NIcookie::DecryptIcookie(encryptedIcookie, /*canThrow=*/ false);
                    maybeIcookie.Defined()) {
                    HttpHeaders.AddHeader(NIcookie::ICOOKIE_DECRYPTED_HEADER, maybeIcookie.GetRef());
                    if (const THttpInputHeader* header = HttpHeaders.FindHeader(NIcookie::ICOOKIE_DECRYPTED_HEADER)) {
                        Headers.Add(header->Name(), header->Value());
                        CSHeaders.emplace(header->Name(), header->Value());
                        if (auto it = loader->HeaderToHash.find(header->Name()); it != loader->HeaderToHash.end()) {
                            Hodor += it->second + "-";
                        }
                        HodorHash += header->Name();

                        ICookie = header->Value();
                    }
                }
            }
        }

        const TStringBuf t = Headers.Get("X-Start-Time"_sb);

        if (!Hodor.empty()) {
            Hodor.pop_back();
        }
        if (!HodorHash.empty()) {
            HodorHash = ToString(FnvHash<ui64>(HodorHash));
        }

        if (t.empty()) {
            ArrivalTime = TInstant::Now();
        } else {
            ArrivalTime = GetRequestTime(t);
        }
        CurrentTimestamp = ArrivalTime.Seconds() % 86400;

        // CAPTCHA-460
        try {
            TMaybe<TMeasureDuration> md;
            if (readStats != nullptr) {
                md.ConstructInPlace(*readStats);
            }
            /* This is a copy-pasted implementation of IInputStream::ReadAll.
             * In case of socket timeout it allows us to save partially read data in
             * a log for further investigation. */
            TStringOutput so(ContentData);
            TransferData(&input, &so);
        } catch (const TSystemError& e) {
            if (IsSocketTimeout(e)) {
                if (failOnReadRequestTimeout || ContentData.empty()) {
                    ythrow TTimeoutException() << "Socket timeout (" << e.Status() << "). "
                                               << CurrentExceptionMessage() << ". "
                                               << "Data read:\n" << ContentData;
                }
            } else {
                throw;
            }
        } catch (const yexception& e) {
            if (isWrappedRequest) {
                // CAPTCHA-2602
                // может прийти невалидный контен от клиента (например, с невалидным кодированием), это не повод пропускать запрос
                // также балансер может отрезать часть контента
                ContentData = "";
            } else {
                throw;
            }
        }

        HostWithPort = GetHost(Headers);

        TStringBuf port;
        HostWithPort.Split(':', Host, port);
        Tld = GetTldFromHost(Host);
        Lang = GetLangFromHost(Host);

        if (ANTIROBOT_DAEMON_CONFIG.DebugOutput) {
            DebugOutput();
        }
    }

    void THttpInfo::PrintData(IOutputStream& os, bool forceMaskCookies) const {
        if (forceMaskCookies) {
            WriteMaskedFirstLine(os);
        } else {
            os << FirstLine;
        }

        os << "\r\n"_sb;

        PrintHeaders(os, forceMaskCookies);

        if (!ContentData.empty())
            os << "\r\n"_sb << ContentData;
    }

    void THttpInfo::PrintHeaders(IOutputStream& os, bool forceMaskCookies) const {
        if (!forceMaskCookies) {
            HttpHeaders.OutTo(&os);
            return;
        }

        static const TString cookieHeaderName = "cookie";
        const THttpInputHeader* cookieHeader = nullptr;
        for (auto& header : HttpHeaders) {
            if (AsciiEqualsIgnoreCase(header.Name(), cookieHeaderName)) {
                cookieHeader = &header;
            } else {
                header.OutTo(&os);
            }
        }

        if (cookieHeader == nullptr) {
            return;
        }

        TString maskedCookies;

        maskedCookies.reserve(cookieHeader->Value().length());

        TCookiesAppender appender = {&maskedCookies};

        ScanKeyValue<true, ';', '='>(cookieHeader->Value(), appender);

        THttpInputHeader maskedHeader(cookieHeader->Name(), maskedCookies);
        maskedHeader.OutTo(&os);
    }

    void THttpInfo::SerializeTo(NCacheSyncProto::TRequest& serializedRequest) const {
        serializedRequest.SetRequesterAddr(RequesterAddr);
        serializedRequest.SetRandomParam(RandomParam);

        TStringOutput so(*serializedRequest.MutableRequest());
        PrintData(so, /* forceMaskCookies := */ false);
    }

    void THttpInfo::DebugOutput() {
        Trace("\n");
        Trace("userContent length: %d\n", ContentData.size());
        Trace("user request: '%s'\n", FirstLine.c_str());
        Trace("user addr: '%s'\n", RequesterAddr.c_str());
        Trace("headers from user:\n");

        PrintHeaders(Cerr, /* forceMaskCookies := */ true);

        Cerr << "User post data:" << Endl
            << ContentData << Endl
            << "End user post data" << Endl;
    }

    void THttpInfo::WriteMaskedFirstLine(IOutputStream& out) const {
        if (ANTIROBOT_DAEMON_CONFIG.JsonConfig[HostType].CgiSecrets.empty()) {
            out << FirstLine;
            return;
        }

        const TStringBuf firstLineRef(FirstLine);
        TStringBuf method, afterMethod;

        if (!firstLineRef.TrySplit(' ', method, afterMethod)) {
            out << firstLineRef;
            return;
        }

        out << method << ' ';

        TStringBuf doc, afterDoc;

        if (!afterMethod.TrySplit(' ', doc, afterDoc)) {
            out << afterMethod;
            return;
        }

        WriteMaskedUrl(HostType, doc, out);
        out << ' ' << afterDoc;
    }

    TFullReqInfo::TFullReqInfo(
        THttpInput& input,
        const TString& wrappedRequest,
        const TString& requesterAddr,
        const TReloadableData& reloadableData,
        const TPanicFlags& panicFlags,
        TSpravkaIgnorePredicate spravkaIgnorePredicate,
        const TRequestGroupClassifier* groupClassifier,
        const TEnv* env,
        TMaybe<TString> uniqueKey,
        TVector<TExpInfo> experimentsHeader,
        TMaybe<ui64> randomParam
    )
        : THttpInfo(input, requesterAddr, nullptr, false, true) // TODO: pass TimeStats and failOnReadRequestTimeout
    {
        if (!ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService && RequestProtocol == "http/1.1"_sb && !Headers.Get("Expect"_sb).empty()) {
            ythrow TBadExpect();
        }
        {
            TStringStream os;
            PrintData(os, /* forceMaskCookies := */ false);
            Request = os.Str();
        }
        const TStringBuf& realIp = Headers.Get("X-Forwarded-For-Y"_sb);
        if (!realIp) {
            if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService && ANTIROBOT_DAEMON_CONFIG.Local) {
                RawAddr.FromString("127.0.0.1");
            } else {
                RawAddr = TAddr();
            }
        } else {
            if (reloadableData.TurboProxyIps.Get()->Contains(TAddr(realIp))) {
                const TStringBuf& realRealIp = Headers.Get("X-Forwarded-For"_sb);
                RawAddr.FromString(!!realRealIp ? realRealIp : realIp);
            } else if (reloadableData.UaProxyIps.Get()->Contains(TAddr(realIp))) {
                const TStringBuf& realRealIp = Headers.Get("Shadow-X-Forwarded-For"_sb);
                RawAddr.FromString(!!realRealIp ? realRealIp : realIp);
            } else {
                RawAddr.FromString(realIp);
            }
        }

        RandomParam = randomParam ? *randomParam : RandomNumber<ui64>();

        if (auto hostType = GetHostType(*this)) {
            HostType = *hostType;
        } else {
            HostType = HOST_OTHER;
            HasUnknownServiceHeader = true;
        }

        ReqId = Headers.Get("X-Req-Id"_sb);
        ServiceReqId = CalcServiceReqid(HostType, ReqId, Headers);
        Scheme = IsTrue(Headers.Get("X-Yandex-HTTPS"_sb)) ? EScheme::HttpSecure
                                                                  : EScheme::Http;

        // from TParsedRequest
        YxSearchPrefs.Init(Cookies);

        YandexUid = GetYandexUid(Cookies);
        if (YandexUid.empty() && HostType == HOST_TRANSLATE) {
            YandexUid = CgiParams.Get("yu");
        }

        if (uniqueKey) {
            UniqueKey = std::move(*uniqueKey);
        } else {
            constexpr std::array<TStringBuf, 4> have{"/showcaptcha", "/checkcaptcha", "/xcheckcaptcha", "/checkcaptchajson"};
            if (IsIn(have, Doc)) {
                if (const auto it = CgiParams.Find("u"); it != CgiParams.end()) {
                    UniqueKey = it->second;
                } else {
                    UniqueKey = CreateGuidAsString();
                }
            } else {
                UniqueKey = CreateGuidAsString();
            }
        }

        const bool disableIsYandex = Cookies.Has(DISABLE_YANDEX);
        const bool disablePrivileged = Cookies.Has(DISABLE_PRIVILEGED);
        const bool disableWhitelist = Cookies.Has(DISABLE_WHITELIST);

        CheckForPartnerRequest(*this); // Changes PartnerAddr here
        const TAddr sender(IsPartnerRequest() ? TAddr(Headers.Get("X-Real-Ip"_sb)) : RawAddr);
        UserAddr = TFeaturedAddr(sender, reloadableData, HostType, disableIsYandex, disablePrivileged, disableWhitelist);

        if (
            const auto langByAcceptLanguage = LanguageByAcceptLanguage(Headers.Get("accept-language"), LANG_UNK, {LANG_RUS});
            langByAcceptLanguage != LANG_UNK && (COMMON_TLD.contains(Tld) || Lang == LANG_UNK)
        ) {
            Lang = langByAcceptLanguage;
        }

        if (
            const auto langByCookie = LanguageByMyCookie(TString(Cookies.Get("my")));
            langByCookie != LANG_UNK
        ) {
            Lang = langByCookie;
        }

        CaptchaReqType = GetCaptchaReqType(Doc);
        ReqType = CalcReqType(*this, HostType, reloadableData);
        ReqGroup = groupClassifier ? groupClassifier->Group(*this) : EReqGroup::Generic;

        VerticalReqGroup = EVerticalReqGroup::Generic;
        if (groupClassifier && IsIn(VERTICAL_TYPES, HostType) && ReqGroup != EReqGroup::Generic) {
            const auto& groupName = groupClassifier->GroupName(HostType, ReqGroup);
            TryFromString(groupName, VerticalReqGroup);
        }

        ClientType = GetClientType(*this, HostType, ReqType, YandexUid);
        InitiallyWasXmlsearch = ReqType == REQ_XMLSEARCH;
        PanicFlags = panicFlags;

        {
            HasAnyICookie = false;
            HasValidICookie = false;
            HasValidOldICookie = false;
            HasAnyFuid = false;
            HasValidFuid = false;
            HasAnyLCookie = false;
            HasValidLCookie = false;
            HasAnySpravka = false;
            HasValidSpravka = false;
            HasValidSpravkaHash = false;
            SpravkaIgnored = false;
            TrustedUser = false;
            Degradation = false;

            try {
                Uid = TUid::FromAddrOrSubnet(UserAddr);
            } catch (yexception&) {
                ythrow TUidCreationFailure() << CurrentExceptionMessage()
                                             << "; Can't parse UserAddr=" << UserAddr << ", RequesterAddr=" << RequesterAddr;
            }
            SpravkaAddr = UserAddr;

            if (!ICookie.empty()) {
                HasAnyICookie = true;
                NIcookie::TIcookieDataWithRaw iCookie;
                if (NIcookie::ParseDecryptedIcookieData(ICookie, iCookie)) {
                    HasValidICookie = true;
                    const auto cookieCreated = TInstant::Seconds(iCookie.IcookieData.Timestamp);
                    CookieAge = (ArrivalTime - cookieCreated).Seconds();
                    if (cookieCreated + ANTIROBOT_DAEMON_CONFIG.MinICookieAge < ArrivalTime) {
                        HasValidOldICookie = true;
                        if (ANTIROBOT_DAEMON_CONFIG.AuthorizeByICookie) {
                            Uid = TUid::FromICookie(iCookie);
                        }
                    }
                }

                // fill trusted user from ICookie or YandexUid
                if (ui64 uid; TryFromString<ui64>(ICookie, uid) && reloadableData.TrustedUsers.Get()->Has(uid)) {
                    TrustedUser = true;
                }
            } else if (!YandexUid.empty()) {
                if (CookieAge == 0.0 && YandexUid.size() > 10) {
                    TStringBuf ts = YandexUid.Tail(YandexUid.size() - 10);

                    if (ui64 cookieCreated; TryFromString<ui64>(ts, cookieCreated)) {
                        CookieAge = (ArrivalTime - TInstant::Seconds(cookieCreated)).Seconds();
                    }
                }
                if (ui64 uid; TryFromString<ui64>(YandexUid, uid) && reloadableData.TrustedUsers.Get()->Has(uid)) {
                    TrustedUser = true;
                }
            }
            if (ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HostType, Tld).DecodeFuidEnabled) {
                if (const auto fuidStr = Cookies.Get(TFlashCookie::Name); !fuidStr.empty()) {
                    HasAnyFuid = true;
                    TFlashCookie fuid;
                    if (fuid.Parse(fuidStr) && fuid.Time() + ANTIROBOT_DAEMON_CONFIG.MinFuidAge < ArrivalTime) {
                        HasValidFuid = true;

                        if (ANTIROBOT_DAEMON_CONFIG.AuthorizeByFuid) {
                            Uid = TUid::FromFlashCookie(fuid);
                            SpravkaTime = fuid.Time();
                        }
                    }
                }
            }

            LCookieUid = 0;
            if (ANTIROBOT_DAEMON_CONFIG.AuthorizeByLCookie) {
                auto keychain = reloadableData.LKeychain.Get();
                if (keychain) {
                    const auto cookieRange = Cookies.EqualRange("L");
                    if (cookieRange.first != Cookies.end()) {
                        HasAnyLCookie = true;

                        const TMaybe<NLCookie::TLCookie>& cookie = NLCookie::TryParse(cookieRange.first->second, *keychain);
                        if (cookie.Defined()) {
                            HasValidLCookie = true;
                            const NLCookie::TLCookie& lcookie = cookie.GetRef();
                            Uid = TUid::FromLCookie(lcookie);
                            LCookieUid = lcookie.Uid;
                            SpravkaTime = TInstant::Seconds(lcookie.Timestamp);
                        }
                    }
                }
            }

            if (ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HostType, Tld).PreviewIdentTypeEnabled) {
                if (!Y_UNLIKELY(PanicFlags.IsPanicPreviewIdentTypeEnabledSet(HostType))) {
                    auto userAgentType = PreviewAgentType();
                    if (userAgentType != EPreviewAgentType::UNKNOWN) {
                        try {
                            Uid = TUid::FromAddrOrSubnetPreview(UserAddr, userAgentType);
                        } catch (yexception&) {
                            ythrow TUidCreationFailure() << CurrentExceptionMessage()
                                                         << "; Can't parse UserAddr=" << UserAddr << ", RequesterAddr=" << RequesterAddr;
                        }
                    }
                }
            }

            const TStringBuf domain = GetCookiesDomainFromHost(Host);

            TSpravka spravka;
            TSpravka::ECookieParseResult parseSpravkaResult;

            if (ReqType == REQ_SITESEARCH && IsTrue(CgiParams.Get("html"))) {
                parseSpravkaResult = spravka.ParseCGI(CgiParams, domain);
            } else {
                parseSpravkaResult = spravka.ParseCookies(Cookies, domain);
                if (parseSpravkaResult == TSpravka::ECookieParseResult::NotFound && HostType == HOST_TRANSLATE) {
                    parseSpravkaResult = spravka.ParseCGI(CgiParams, domain);
                }
            }

            if (parseSpravkaResult != TSpravka::ECookieParseResult::NotFound) {
                HasAnySpravka = true;

                if (
                    parseSpravkaResult == TSpravka::ECookieParseResult::Valid
                ) {
                    HasValidSpravkaHash = true;

                    if (!SpravkaExpired(&spravka, ArrivalTime)) {
                        auto spravkaUid = TUid::FromSpravka(spravka);
                        if (spravkaIgnorePredicate(spravkaUid, HostType)) {
                            SpravkaIgnored = true;
                        } else {
                            HasValidSpravka = true;
                            SpravkaAddr = spravka.Addr;
                            Uid = spravkaUid;
                            SpravkaTime = spravka.Time;
                            if (HostType == HOST_WEB) {
                                Degradation = spravka.Degradation.Web;
                            } else if (HostType == HOST_USLUGI) {
                                Degradation = spravka.Degradation.Uslugi;
                            } else if (HostType == HOST_AUTORU) {
                                Degradation = spravka.Degradation.Autoru;
                            } else if (IsIn(MARKET_TYPES, HostType)) {
                                Degradation = spravka.Degradation.Market;
                            }
                        }
                    }
                }
            }
        }

        if (!experimentsHeader.empty()) {
            ExperimentsHeader = std::move(experimentsHeader);
        } else if (env && !AtomicGet(env->AntirobotDisableExperimentsFlag.Enable) && HostType == HOST_MARKET) {
            bool gotIntoAnExperiment = false;
            NUserSplit::TBucket bucket;
            ui32 testId;

            const TAntirobotExperiment antirobotMarketExperiment(474970, 474971, "Qk2zyvl", 3, 3);
            std::tie(gotIntoAnExperiment, testId, bucket) = antirobotMarketExperiment.GetExpInfo(HasValidICookie ? ICookie : RawAddr.ToString());
            MarketExpSkipCaptcha = gotIntoAnExperiment;
            if (testId != 0) {
                ExperimentsHeader.push_back({testId, static_cast<i8>(bucket)});
            }
        }

        IsSearch = IsReqTypeSearch(ReqType, this->Doc);

        ForceShowCaptcha = Cookies.Has("YX_SHOW_CAPTCHA"_sb) || HasMagicRequestForCaptcha();

        if (IsPartnerRequest() && ReqType == REQ_XMLSEARCH) {
            ApplyHackForXmlPartners();
        }

        CalcBlockCategory();

        if (ANTIROBOT_DAEMON_CONFIG.DebugOutput) {
            DebugOutput();
        }

        HackTrailingContentForVerochka(wrappedRequest);
        CaptchaRequest = TCaptchaRequest(env, *this);
        if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
            Lang = CaptchaRequest.Lang;
        }
        HasJws = Headers.Has("X-Jws") || Headers.Has(YANDEX_JWS_HEADER);
        HasYandexTrust = Headers.Has("Yandex-Trust");

        CbbPanicMode = PanicFlags.IsPanicCbbSet(HostType);

        if (env) {
            ProcessAntirobotCookie(*env, this);

            FillJws(env);
            YandexTrustState = GetYandexTrustState(*this);

            if (HostType == HOST_APIAUTO || HostType == HOST_REALTY) {
                HasValidAutoRuTamper = CheckAutoRuTamper(Headers, CgiParams, env->AutoRuTamperSalt);
            }

            InRobotSet = env->Robots->Contains(HostType, Uid);
        }

        SupportsBrotli = input.AcceptEncoding("br");
        SupportsGzip = input.AcceptEncoding("gzip");
    }

    void TFullReqInfo::FillJws(const TEnv* env) {
        TStringBuf jwsStr;

        if (const auto narwhalJws = Headers.Get(YANDEX_JWS_HEADER); !narwhalJws.empty()) {
            jwsStr = narwhalJws;
            std::tie(JwsPlatform, JwsState) = CheckNarwhalJws(std::string(narwhalJws), *env);
        } else if (const auto marketJws = Headers.Get("X-Jws"); !marketJws.empty()) {
            jwsStr = marketJws;
            std::tie(JwsPlatform, JwsState) =
                CheckMarketJws(std::string(marketJws), env->MarketJwsKey);
        }

        auto [jwsHashLow, jwsHashHigh] = CityHash128(jwsStr);
        jwsHashLow = HostToLittle(jwsHashLow);
        jwsHashHigh = HostToLittle(jwsHashHigh);

        static_assert(sizeof(jwsHashLow) + sizeof(jwsHashHigh) == sizeof(JwsHash));
        std::memcpy(&JwsHash[0], &jwsHashLow, sizeof(jwsHashLow));
        std::memcpy(&JwsHash[sizeof(jwsHashLow)], &jwsHashHigh, sizeof(jwsHashHigh));
    }

    void TFullReqInfo::HackTrailingContentForVerochka(const TString& wrappedRequest) {
        if (!wrappedRequest) {
            return;
        }

        if (RequestMethod != "post"sv) {
            return;
        }

        if (Doc != "/checkcaptcha" && Doc != "/tmgrdfrendc") {
            return;
        }

        if (Headers.Has("Content-length") || !ContentData.Empty()) {
            return;
        }

        TStringBuf prefix, content;
        if (TStringBuf(wrappedRequest).TrySplit("\r\n\r\n", prefix, content)) {
            ContentData = TString{content};
        }
    }

    void TFullReqInfo::ApplyHackForXmlPartners() {
        ClientType = CLIENT_XML_PARTNER;
        HasValidFuid = true;
        ReqType = REQ_YANDSEARCH;
        HostType = HOST_WEB;
        IsSearch = true;
    }

    void TFullReqInfo::DebugOutput() {
        Cerr << "Trusted uid: " << static_cast<int>(Uid.Trusted())
            << ", HOST_TYPE: " << HostType
            << ", REQ_TYPE: " << ReqType
            << ", BLOCK_CATEGORY: " << BlockCategory
            << ", CLIENT_TYPE: " << ClientType
            << ", GeoRegion: " << UserAddr.GeoRegion()
            << Endl;
    }

    void TFullReqInfo::CalcBlockCategory() {
        if (UserAddr.IsWhitelisted()) {
            BlockCategory = BC_ANY_FROM_WHITELIST;
            return;
        }

        if (IsSearch) {
            BlockCategory = HasValidSpravka ? BC_SEARCH_WITH_SPRAVKA : BC_SEARCH;
            return;
        }

        if (!MayBanFor()) {
            BlockCategory = BC_NON_SEARCH;
        }
    }

    TSpravkaIgnorePredicate GetSpravkaIgnorePredicate(const TEnv& env) {
        if (!ANTIROBOT_DAEMON_CONFIG.SpravkaIgnoreIfInRobotSet) {
            return [] (const TUid&, EHostType) { return false; };
        }

        return [&env] (const TUid& spravkaUid, EHostType hostType) {
            return env.Robots->Contains(hostType, spravkaUid);
        };
    }

    TSpravkaIgnorePredicate GetEmptySpravkaIgnorePredicate() {
        return [](const TUid, EHostType) {
            return false;
        };
    }

    THolder<TFullReqInfo> ParseFullreq(const NCacheSyncProto::TRequest& serializedRequest, const TEnv& env) {
        TStringInput si(serializedRequest.GetRequest());
        THttpInput httpInput(&si);
        auto spravkaIgnorePredicate = [&serializedRequest](const TUid& uid, EHostType) {
            if (uid.Ns != TUid::SPRAVKA) {
                return false;
            }
            return serializedRequest.GetSpravkaIgnored();
        };

        TVector<TRequest::TExpInfo> expHeader;
        for (auto& x : serializedRequest.GetExperimentsTestId()) {
            expHeader.push_back({x, 0});
        }

        return MakeHolder<TFullReqInfo>(
            httpInput,
            "",
            serializedRequest.GetRequesterAddr(),
            env.ReloadableData,
            env.PanicFlags,
            spravkaIgnorePredicate,
            &env.ReqGroupClassifier,
            &env,
            serializedRequest.GetUniqueKey(),
            expHeader,
            serializedRequest.GetRandomParam()
        );
    }

    THolder<TRequest> CreateDummyParsedRequest(
        const TString& request,
        const TRequestClassifier& classifier,
        const TRequestGroupClassifier& groupClassifier,
        const TString& geodataBinPath
    ) {
        static TReloadableData reloadable;
        reloadable.RequestClassifier.Set(classifier);
        if (!geodataBinPath.empty()) {
            reloadable.GeoChecker.Set(TGeoChecker(geodataBinPath));
        }
        TStringInput input(request);
        THttpInput httpInput(&input);
        // create here the request with fuctions for non-panic mode (will be used mostly in unit tests)
        return MakeHolder<TFullReqInfo>(
            httpInput,
            "",
            "",
            reloadable,
            TPanicFlags::CreateFake(),
            GetEmptySpravkaIgnorePredicate(),
            &groupClassifier,
            nullptr
        );
    }

    void SplitUri(TStringBuf& req, TStringBuf& doc, TStringBuf& cgi) {
        req = SkipDomain(req);
        while (req.size() > 1 && req[1] == '/') {
            req.Skip(1);
        }

        req.Split('?', doc, cgi);
        while (doc.size() > 1 && doc.back() == '/') {
            doc.Chop(1);
        }
    }

    void WriteMaskedUrl(EHostType service, TStringBuf url, IOutputStream& out) {
        TStringBuf beforeQuestMark, afterQuestMark;

        if (
            ANTIROBOT_DAEMON_CONFIG.JsonConfig[service].CgiSecrets.empty() ||
            !url.TrySplit('?', beforeQuestMark, afterQuestMark)
        ) {
            out << url;
            return;
        }

        out << beforeQuestMark << '?';

        for (size_t i = 0; i < afterQuestMark.size();) {
            WriteMaskedCgiParam(service, afterQuestMark, out, i);
        }
    }

    void WriteMaskedCgiParam(EHostType service, TStringBuf url, IOutputStream& out, size_t& i) {
        const size_t keyStart = i;

        while (
            i < url.size() &&
            url[i] != '&' &&
            url[i] != '='
        ) {
            ++i;
        }

        const auto key = url.SubStr(keyStart, i - keyStart);
        out << key;

        if (i == url.size()) {
            return;
        }

        const char sep = url[i];
        out << sep;
        ++i;

        if (sep == '&') {
            return;
        }

        const size_t valueStart = i;

        while (i < url.size() && url[i] != '&') {
            ++i;
        }

        if (!ANTIROBOT_DAEMON_CONFIG.JsonConfig[service].CgiSecrets.contains(key)) {
            out << url.SubStr(valueStart, i - valueStart);
        }

        if (i < url.size()) {
            out << '&';
            ++i;
        }
    }
}
