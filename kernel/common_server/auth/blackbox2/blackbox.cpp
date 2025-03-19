#include "blackbox.h"
#include "auth.h"

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/json/builder.h>
#include <kernel/common_server/library/json/cast.h>
#include <kernel/common_server/library/network/data/data.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/util/algorithm/ptr.h>
#include <kernel/common_server/util/threading.h>
#include <kernel/common_server/auth/common/tvm_config.h>

#include <library/cpp/auth_client_parser/cookie.h>
#include <library/cpp/auth_client_parser/oauth_token.h>
#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/tvmauth/client/facade.h>

#include <util/string/vector.h>

namespace {
    TNamedSignalSimple BlackboxRequest("blackbox-request");
    TNamedSignalSimple BlackboxOk("blackbox-ok");
    TNamedSignalSimple BlackboxException("blackbox-exception");
    TNamedSignalSimple BlackboxNull("blackbox-null");
    TNamedSignalSimple BlackboxTimeout("blackbox-timeout");
    TNamedSignalEnum<NBlackbox2::TSessionResp::EStatus> BlackboxStatus("blackbox", EAggregationType::Sum, "dmmm");
    TNamedSignalHistogram BlackboxTimes("blackbox-times", NRTProcHistogramSignals::IntervalsRTLineReply);
}

TBlackbox2AuthConfig::TBlackbox2AuthConfig(const TString& name)
    : IAuthModuleConfig(name)
{
}

TBlackbox2AuthConfig::~TBlackbox2AuthConfig() {
}

void TBlackbox2AuthConfig::DoInit(const TYandexConfig::Section* section) {
    CHECK_WITH_LOG(section);
    const auto& directives = section->GetDirectives();

    AuthMethod = directives.Value("AuthMethod", AuthMethod);
    IgnoreDeviceId = directives.Value("IgnoreDeviceId", IgnoreDeviceId);
    NeedInClientTicket = directives.Value("NeedInClientTicket", NeedInClientTicket);
    Scopes = SplitString(directives.Value("Scopes", JoinStrings(Scopes, TStringBuf(","))), ",");

    SenderName = directives.Value("SenderName", SenderName);
    AssertCorrectConfig(!!SenderName, "empty 'SenderName' field");

    CookieHost = directives.Value("CookieHost", CookieHost);
    AssertCorrectConfig(!CookieHost.empty(), "empty 'CookieHost' field");
}

void TBlackbox2AuthConfig::DoToString(IOutputStream& os) const {
    os << "AuthMethod: " << AuthMethod << Endl;
    os << "SenderName: " << SenderName << Endl;
    os << "CookieHost: " << CookieHost << Endl;
    os << "IgnoreDeviceId: " << IgnoreDeviceId << Endl;
    os << "NeedInClientTicket: " << NeedInClientTicket << Endl;
    os << "Scopes: " << JoinStrings(Scopes, TStringBuf(",")) << Endl;
}

THolder<IAuthModule> TBlackbox2AuthConfig::DoConstructAuthModule(const IBaseServer* server) const {
    if (!server) {
        ERROR_LOG << "nullptr IBaseServer" << Endl;
        return nullptr;
    }
    if (!Client) {
        auto sender = server->GetSenderPtr(SenderName);
        if (!sender) {
            ERROR_LOG << "cannot find sender for SenderName " << SenderName << Endl;
            return nullptr;
        }
        auto client = MakeAtomicShared<NCS::TBlackboxClient>(sender);
        client->SetCookieHost(CookieHost);
        client->SetScopes(Scopes);
        client->SetUserTicketsEnabled(NeedInClientTicket);
        auto guard = Guard(ClientLock);
        Client = std::move(client);
    }
    return MakeHolder<TBlackbox2AuthModule>(*this, Client);
}

TBlackbox2AuthModule::TBlackbox2AuthModule(const TBlackbox2AuthConfig& config, TAtomicSharedPtr<NCS::TBlackboxClient> client)
    : Config(config)
    , Client(client)
{
}

IAuthInfo::TPtr TBlackbox2AuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    if (!requestContext) {
        return MakeAtomicShared<TBlackboxAuthInfo>("null RequestContext");
    }
    if (!Client) {
        return MakeAtomicShared<TBlackboxAuthInfo>("null Blackbox client");
    }

    const TBaseServerRequestData& rd = requestContext->GetBaseRequestData();
    TStringBuf auth = rd.HeaderInOrEmpty("Authorization");
    TStringBuf cookie = rd.HeaderInOrEmpty("Cookie");
    TStringBuf userIp = NUtil::GetClientIp(rd);

    TStringBuf sessionId;
    if (cookie) {
        THttpCookies cookies(cookie);
        sessionId = cookies.Get("Session_id");
    }

    NThreading::TFuture<NCS::TBlackboxClient::TResponsePtr> asyncResponse;
    TInstant start = Now();
    switch (Config.GetAuthMethod()) {
    case TBlackbox2AuthConfig::EAuthMethod::Any:
        if (auth) {
            asyncResponse = MakeOAuthRequest(auth, userIp);
        } else if (sessionId) {
            asyncResponse = MakeSessionIdRequest(sessionId, userIp);
        } else {
            TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder("type", "NoCredentials"));
        }
        break;
    case TBlackbox2AuthConfig::EAuthMethod::Cookie:
        if (sessionId) {
            asyncResponse = MakeSessionIdRequest(sessionId, userIp);
        } else {
            TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder("type", "NoSessionId"));
        }
        break;
    case TBlackbox2AuthConfig::EAuthMethod::OAuth:
        if (auth) {
            asyncResponse = MakeOAuthRequest(auth, userIp);
        } else {
            TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder("type", "NoOAuthToken"));
        }
        break;
    }
    if (!asyncResponse.Initialized()) {
        return MakeAtomicShared<TBlackboxAuthInfo>("initialization error");
    }

    BlackboxRequest.Signal(1);
    if (!asyncResponse.Wait(requestContext->GetRequestDeadline())) {
        BlackboxTimeout.Signal(1);
        TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder
            ("type", "WaitTimeout")
            ("elapsed", NJson::ToJson(Now() - start)
        ));
        return MakeAtomicShared<TBlackboxAuthInfo>("blackbox wait timeout");
    }
    TInstant finish = Now();
    TDuration duration = finish - start;
    BlackboxTimes.Signal(duration.MilliSeconds());

    if (!asyncResponse.HasValue()) {
        BlackboxException.Signal(1);
        auto message = NThreading::GetExceptionMessage(asyncResponse);
        TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder
            ("type", "Exception")
            ("message", message)
        );
        return MakeAtomicShared<TBlackboxAuthInfo>(message);
    }
    BlackboxOk.Signal(1);

    auto bbResponse = asyncResponse.ExtractValue();
    if (!bbResponse) {
        BlackboxNull.Signal(1);
        TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder
            ("type", "NullSessionIdResponse")
        );
        return MakeAtomicShared<TBlackboxAuthInfo>("NullSessionIdResponse");
    }

    auto sessionResponse = std::dynamic_pointer_cast<NBlackbox2::TSessionResp>(bbResponse);
    auto status = sessionResponse ? sessionResponse->Status() : NBlackbox2::TSessionResp::Valid;
    BlackboxStatus.Signal(status, 1);
    if (status != NBlackbox2::TSessionResp::Valid && status != NBlackbox2::TSessionResp::NeedReset) {
        const auto message = ToString(bbResponse->Message());
        const auto error = message ? message : (TStringBuilder() << "BadSessionIdResponse" << ' ' << static_cast<int>(status));
        TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder
            ("type", "BadSessionIdResponse")
            ("message", message)
            ("status", static_cast<int>(status))
        );
        return MakeAtomicShared<TBlackboxAuthInfo>(error);
    }

    auto info = Client->Parse(*bbResponse);
    info.IgnoreDeviceId = Config.ShouldIgnoreDeviceId();
    return MakeAtomicShared<TBlackboxAuthInfo>(std::move(info));
}

NThreading::TFuture<NCS::TBlackboxClient::TResponsePtr> TBlackbox2AuthModule::MakeOAuthRequest(TStringBuf authorization, TStringBuf userIp) const {
    constexpr TStringBuf OAuthPrefix = "OAuth ";
    const TStringBuf token = StripString(authorization.SubStr(OAuthPrefix.size()));

    NAuthClientParser::TOAuthToken parser;
    if (!parser.Parse(token)) {
        DEBUG_LOG << "ill-formed token: " << token << Endl;
        TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder
            ("type", "IllFormedToken")
            ("token", token)
        );
        return {};
    }

    return Checked(Client)->OAuthRequest(token, userIp);
}

NThreading::TFuture<NCS::TBlackboxClient::TResponsePtr> TBlackbox2AuthModule::MakeSessionIdRequest(TStringBuf sessionId, TStringBuf userIp) const {
    NAuthClientParser::TZeroAllocationCookie parser;
    NAuthClientParser::EParseStatus status = parser.Parse(sessionId);
    if (status != NAuthClientParser::EParseStatus::RegularMayBeValid) {
        DEBUG_LOG << "ill-formed Session_id: " << sessionId << ' ' << static_cast<int>(status) << Endl;
        TFLEventLog::ModuleLog("BlackboxError", TLOG_INFO)("data", NJson::TMapBuilder
            ("type", "IllFormedSessionId")
            ("session_id", sessionId)
            ("status", static_cast<int>(status))
        );
        return {};
    }

    return Checked(Client)->SessionIdRequest(sessionId, userIp);
}

TBlackbox2AuthConfig::TFactory::TRegistrator<TBlackbox2AuthConfig> TBlackbox2AuthConfig::Registrator("blackbox2");
