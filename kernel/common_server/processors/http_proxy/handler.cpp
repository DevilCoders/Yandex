#include "handler.h"
#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>
#include <kernel/common_server/util/network/http_request.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/string_utils/url/url.h>
#include <kernel/common_server/processors/common/forward/agent.h>
#include <kernel/common_server/processors/common/forward/direct.h>

namespace {
    constexpr TStringBuf CONNECT("CONNECT");
    constexpr TStringBuf DIGEST_PREFIX("Digest ");

    TStringBuf GetRequestType(IReplyContext::TPtr context) {
        return context->GetBuf().Empty() ? "GET" : "POST";
    }

    void MakeReport(TJsonReport::TGuard& g, const NUtil::THttpReply& reply) {
        for (const auto& h: reply.GetHeaders()) {
            g->GetContextPtr()->AddReplyInfo(h.Name(), h.Value());
        }
        g.SetExternalReportString(reply.ErrorMessage() + reply.Content(), false);
        g.SetCode(reply.Code() ? reply.Code() : HTTP_INTERNAL_SERVER_ERROR);
    }

    NNeh::THttpRequest ConstructForwardRequest(IReplyContext::TPtr context, const TStringBuf uri, const TStringBuf targetUrl, const TString& proxyAuth, const TString httpMethod = "GET") {
        NNeh::THttpRequest forwardRequest;
        forwardRequest.SetRequestType(httpMethod);
        forwardRequest.SetUri(TString(uri));
        if (!targetUrl.empty()) {
            forwardRequest.SetTargetUrl(TString(targetUrl));
        }
        forwardRequest.SetCgiData(context->GetCgiParameters().Print());
        if (!context->GetBuf().Empty()) {
            forwardRequest.SetPostData(context->GetBuf());
        }
        for (const auto& [key, value]: context->GetBaseRequestData().HeadersIn()) {
            if (key == "Host" || key == "Content-Length") {
                continue;
            }
            forwardRequest.AddHeader(key, value);
        }
        if (!!proxyAuth) {
            forwardRequest.AddHeader("Proxy-Authorization", proxyAuth);
        }
        return forwardRequest;
    }

    class TSocketGuardPolicy {
    public:
        static inline void Acquire(TSocket* /*t*/) noexcept {
        }

        static inline void Release(TSocket* t) noexcept {
            t->Close();
        }
    };

    void ForwardRequestConnect(TJsonReport::TGuard& g, NExternalAPI::TSender::TPtr proxyClient, const TStringBuf uri, const TStringBuf targetUrl, const TString& proxyAuth) {
        try {
            const TString targetUrlStr = TString(targetUrl);
            const TString uriStr = TString(uri);
            NNeh::THttpRequest authRequest;
            authRequest.SetRequestType(TString(CONNECT));
            authRequest.SetUri(uriStr);
            authRequest.SetTargetUrl(targetUrlStr);
            authRequest.AddHeader("Proxy-Authorization", proxyAuth);
            authRequest.AddHeader("Proxy-Connection", "Keep-Alive");
            const auto& proxyCfg = proxyClient->GetConfig();
            TSocket s(TNetworkAddress(proxyCfg.GetHost(), proxyCfg.GetPort()), proxyCfg.GetRequestConfig().GetGlobalTimeout());
            TGuard<TSocket, TSocketGuardPolicy> gSocket(s);
            const auto timeout = proxyCfg.GetRequestConfig().GetSendingTimeout();
            s.SetSocketTimeout(timeout.Seconds(), timeout.MilliSecondsOfSecond());
            s.SetKeepAlive(true);
            NNeh::THttpRequestBuilder rBuilder(proxyCfg.GetHost(), proxyCfg.GetPort());
            NUtil::THttpReply reply;
            ELogPriority logPriority = ELogPriority::TLOG_INFO;
            if (!NUtil::SendRequest(s, rBuilder.MakeNehMessage(authRequest).Data, reply, false, false) || reply.Code() != HTTP_OK) {
                logPriority = ELogPriority::TLOG_ERR;
                MakeReport(g, reply);
            }
            TFLEventLog::Signal("auth_proxy_reply")("&api", proxyClient->GetApiName())("&uri", uri)("target_uri", targetUrl)("&http_code", reply.Code()).SetPriority(logPriority);
            if (logPriority == ELogPriority::TLOG_ERR) {
                return;
            }

            TStringBuf tScheme, tHost;
            ui16 tPort;
            TryGetSchemeHostAndPort(targetUrlStr, tScheme, tHost, tPort);
            NNeh::THttpRequestBuilder oBuilder(TString(tHost), tPort, true);
            auto fwdRequest = ConstructForwardRequest(g->GetContextPtr(), uriStr, "", proxyAuth, TString(GetRequestType(g->GetContextPtr())));
            if (!NUtil::SendRequest(s, oBuilder.MakeNehMessage(fwdRequest).Data, reply, true, true)) {
                logPriority = ELogPriority::TLOG_ERR;
            }
            TFLEventLog::Signal("forward_proxy_reply")("&api", proxyClient->GetApiName())("&uri", uri)("target_uri", targetUrl)("&http_code", reply.Code()).SetPriority(logPriority);
            MakeReport(g, reply);
        } catch (...) {
            g.SetExternalReportString(CurrentExceptionMessage(), false);
            g.SetCode(HTTP_INTERNAL_SERVER_ERROR);
        }
    }

}

namespace NCS {

    class THttpProxyHandler::TAuthRequest : public NExternalAPI::IServiceApiHttpRequest {
        TStringBuf RequestType;
        TStringBuf Uri;
        TStringBuf TargetUrl;
    public:
        TAuthRequest(const TStringBuf requestType, const TStringBuf uri, const TStringBuf targetUrl)
            : RequestType(requestType)
            , Uri(uri)
            , TargetUrl(targetUrl)
        {}

        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            request.SetRequestType(TString(RequestType));
            request.SetUri(TString(Uri));
            request.SetTargetUrl(TString(TargetUrl));
            return true;
        }

        class TResponse : public IServiceApiHttpRequest::IBaseResponse {
        private:
            TString Realm;
            TString Nonce;
            TString Qop;
        protected:
            virtual bool DoParseHttpResponse(const NUtil::THttpReply& reply) override {
                if (const auto* proxyAuthHeader = reply.GetHeaders().FindHeader("Proxy-Authenticate")) {
                    TStringBuf proxyAuth(proxyAuthHeader->Value());
                    if (!proxyAuth.SkipPrefix(DIGEST_PREFIX)) {
                        TFLEventLog::Error("Invalid Proxy-Authenticate")("header_value", proxyAuth);
                        return false;
                    }
                    StringSplitter(proxyAuth).SplitByString(", ").SkipEmpty().Consume([this](const TStringBuf t) {
                        TStringBuf k, v;
                        t.Split('=', k, v);
                        v.SkipPrefix("\"");
                        v.ChopSuffix("\"");
                        if (k == "realm") {
                            Realm = TString(v);
                        }
                        else if (k == "nonce") {
                            Nonce = TString(v);
                        }
                        else if (k == "qop") {
                            Qop = TString(v);
                        }
                        });
                }
                return true;
            }
            virtual bool IsReplyCodeSuccess(const i32 code) const override {
                return code == HTTP_PROXY_AUTHENTICATION_REQUIRED;
            }
        public:
            TString CalcProxyAuthorization(const TString& login, const TString& password, const TStringBuf requestType, const TStringBuf uri, const TStringBuf targetUrl) const {
                //https://ru.wikipedia.org/wiki/Дайджест-аутентификация
                const TString ha1 = MD5::Calc(Join(':', login, Realm, password));
                TStringBuilder uriBuilder;
                uriBuilder << targetUrl;
                if (!uri.StartsWith('/')) {
                    uriBuilder << "/";
                }
                uriBuilder << uri;
                const TString ha2 = MD5::Calc(Join(':', requestType, uriBuilder));
                const TString nc = "00000001";
                const TString cnonce = MD5::Calc(TGUID::Create().AsGuidString());
                const TString responce = MD5::Calc(Join(':', ha1, Nonce, nc, cnonce, Qop, ha2));
                TStringBuilder pa;
                pa << DIGEST_PREFIX << "username=\"" << login << "\", realm=\"" << Realm << "\", nonce=\"" << Nonce << "\", uri=\""
                    << uriBuilder << "\", nc=" << nc << ", cnonce=\"" << cnonce << "\", qop=" << Qop
                    << ", response=\"" << responce << "\"";
                return pa;
            }
        };
    };


    bool THttpProxyHandlerConfig::InitFeatures(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        dir.GetValue("ProxyApiName", ProxyApiName);
        dir.GetValue("ProxyLogin", ProxyLogin);
        dir.GetValue("ProxyPassword", ProxyPassword);
        dir.GetValue("TargetUrl", TargetUrl);
        dir.GetValue("NeedConnect", NeedConnectFlag);
        dir.GetValue("Authorization", AuthorizationFlag);
        return true;
    }

    void THttpProxyHandlerConfig::ToStringFeatures(IOutputStream& os) const {
        os << "ProxyApiName: " << ProxyApiName << Endl;
        os << "ProxyLogin: " << ProxyLogin << Endl;
        os << "ProxyPassword: " << MD5::Calc(ProxyPassword) << Endl;
        os << "TargetUrl: " << TargetUrl << Endl;
        os << "NeedConnect: " << NeedConnectFlag << Endl;
        os << "Authorization: " << AuthorizationFlag << Endl;
    }

    THttpProxyHandler::THttpProxyHandler(const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
        : TBase(config, context, authModule, server)
        , ProxyClient(server->GetSenderPtr(config.GetProxyApiName()))
    {
        auto path = StringSplitter(config.GetHandlerName()).Split('/').SkipEmpty().ToList<TStringBuf>();
        while(!path.empty() && path.back() == "*") {
            path.pop_back();
        }
        UriPrefix = JoinSeq('/', path) + "/";
    }

    TString THttpProxyHandler::GetTypeName() {
        return "http-proxy";
    }

    void THttpProxyHandler::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr /*permissions*/) {
        ReqCheckCondition(!!ProxyClient, HTTP_INTERNAL_SERVER_ERROR, "Invalid handler configuration: there is no ExternalApi '" + Config.GetProxyApiName() + "'");

        TString uriStr = Context->GetUri();
        TStringBuf uri = uriStr;
        ReqCheckCondition(uri.SkipPrefix(UriPrefix), HTTP_INTERNAL_SERVER_ERROR, "Invalid handler configuration: " + TString(uri) + " doesn't start with " + UriPrefix);
        const TStringBuf rType = Config.IsNeedConnect() ? CONNECT : GetRequestType(Context);
        TString proxyAuth;
        if (Config.IsAuthorization()) {
            const auto authResponse = ProxyClient->SendRequest<TAuthRequest>(rType, uri, Config.GetTargetUrl());
            ReqCheckCondition(authResponse.IsSuccess(), authResponse.GetCode(), "cannot auth in proxy");
            proxyAuth = authResponse.CalcProxyAuthorization(Config.GetProxyLogin(), Config.GetProxyPassword(), rType, uri, Config.GetTargetUrl());
        }

        if (!Config.IsNeedConnect() || !Config.IsAuthorization()) {
            NCS::NForwardProxy::TAgent agent(GetServer().GetSenderPtr(Config.GetProxyApiName()), new NCS::NForwardProxy::TDirectConstructor);
            NNeh::THttpRequest fwdRequest = ConstructForwardRequest(Context, uri, Config.GetTargetUrl(), proxyAuth, TString(GetRequestType(Context)));
            agent.ForwardToExternalAPI(g, fwdRequest);
        } else {
            ForwardRequestConnect(g, ProxyClient, uri, Config.GetTargetUrl(), proxyAuth);
        }
    }
}
