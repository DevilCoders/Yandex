#include "turbo_login.h"

#include <quality/functionality/turbo/runtime/common/blackbox_helpers.h>
#include <quality/functionality/turbo/runtime/common/request.h>
#include <quality/functionality/turbo/runtime/common/util.h>
#include <quality/functionality/turbo/urls_lib/cpp/lib/common.h>
#include <quality/functionality/turbo/urls_lib/cpp/lib/turbo_domain.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <util/stream/trace.h>
#include <util/system/maxlen.h>
#include <util/string/hex.h>
#include <library/cpp/string_utils/url/url.h>

#include <contrib/libs/openssl/include/openssl/hmac.h>


namespace {
    const TStringBuf UNAUTHORIZED_SIGNATURE_PREFIX = "u:";
    const TStringBuf AUTHORIZED_SIGNATURE_PREFIX = "a:";
    const TStringBuf SESSION_ID_COOKIE = "session_id";
    const TStringBuf DOMAIN_CGI_PARAMETER = "domain";
    const TStringBuf ACCESS_CONTROL_ALLOW_ORIGIN_HEADER = "Access-Control-Allow-Origin";
    const TStringBuf ACCESS_CONTROL_ALLOW_METHODS_HEADER = "Access-Control-Allow-Methods";
    const TStringBuf ACCESS_CONTROL_ALLOW_METHODS_VALUE = "GET, OPTIONS";
    const TStringBuf ACCESS_CONTROL_ALLOW_CREDENTIALS_HEADER = "Access-Control-Allow-Credentials";
    const TStringBuf ACCESS_CONTROL_ALLOW_CREDENTIALS_VALUE = "true";
    const TStringBuf ACCESS_CONTROL_ALLOW_HEADERS_HEADER = "Access-Control-Allow-Headers";
    const TStringBuf ACCESS_CONTROL_ALLOW_HEADERS_VALUE = "*";
    const TStringBuf BLACKBOX_REQUEST_CONTEXT_TYPE = "blackbox_http_request";
    const TStringBuf BLACKBOX_RESPONSE_CONTEXT_TYPE = "blackbox_http_response";
    const TStringBuf HTTP_RESPONSE_CONTEXT_TYPE = "http_response";
    const TStringBuf SET_COOKIE_HEADER_VALUE = "Set-Cookie";
    const TStringBuf RU_YANDEX_TOP_LEVEL_DOMAIN = "ru";
    const TStringBuf METHOD_GET = "get";
    const TStringBuf METHOD_OPTIONS = "options";

    const TStringBuf RESPONSE_BEGIN = "{\"turbo_uid\":\"";
    const TStringBuf RESPONSE_END = "\"}";

    TString WrapTurboLoginInJson(TStringBuf turboLogin) {
        TString result;
        result.reserve(RESPONSE_BEGIN.length() + turboLogin.length() + RESPONSE_END.length());
        result += RESPONSE_BEGIN;
        result += turboLogin;
        result += RESPONSE_END;
        return result;
    }
} // namespace


namespace NTurboLogin {
    TString GenerateTurboLogin(TStringBuf prefix, TStringBuf domain, TStringBuf yandexValue, const TVector<ui8>& signatureSecretKey) {
        TStackVec<unsigned char, URL_MAX, true> signedValue;
        signedValue.insert(signedValue.end(), domain.begin(), domain.end());
        signedValue.push_back(':');
        signedValue.insert(signedValue.end(), yandexValue.begin(), yandexValue.end());

        const EVP_MD* md_algo = ::EVP_sha256();
        unsigned int tag_size = ::EVP_MD_size(md_algo);

        TStackVec<char, 100, true> hmac;  // needed size is 32
        hmac.resize(tag_size);

        ::HMAC(md_algo, signatureSecretKey.data(), signatureSecretKey.size(), signedValue.data(), signedValue.size(), reinterpret_cast<unsigned char*>(hmac.begin()), &tag_size);

        TString signature;
        signature.reserve(prefix.size() + (hmac.size() << 1));
        signature += prefix;
        signature += HexEncode(TStringBuf(hmac.begin(), hmac.end()));
        signature.to_lower();
        return signature;
    }


    TString GenerateUnauthorizedTurboLogin(TStringBuf domain, TStringBuf yandexUid, const TVector<ui8>& signatureSecretKey) {
        return GenerateTurboLogin(UNAUTHORIZED_SIGNATURE_PREFIX, domain, yandexUid, signatureSecretKey);
    }

    TString GenerateAuthorizedTurboLogin(TStringBuf domain, TStringBuf passportUid, const TVector<ui8>& signatureSecretKey) {
        return GenerateTurboLogin(AUTHORIZED_SIGNATURE_PREFIX, domain, passportUid, signatureSecretKey);
    }

    void ProcessTurboLogin(NAppHost::IServiceContext& ctx, const TVector<ui8>& signatureSecretKey) {
        try {
            NTurbo::TRequest request{ctx};
            if (!NTurbo::IsHttpsRequest(request)) {
                NTurbo::MakeRedirectToHttps(ctx, request.GetHostname(), request.GetUri());
                return;
            }

            TStringBuf refererHost;
            TStringBuf refererPath;
            SplitUrlToHostAndPath(request.GetReferer(), refererHost, refererPath);
            const bool isCorsMatched =
                refererHost.empty() ||
                NTurbo::IsTurbopagesHost(refererHost) ||
                NTurbo::IsYandexHost(refererHost) ||
                NTurbo::IsYandexSportHost(refererHost);

            TString method = request.GetMethod();
            method.to_lower();
            if (method == METHOD_OPTIONS) {
                NTurbo::THttpResponseBuilder httpResponse;
                httpResponse
                    .SetStatusCode(200)
                    .AddHeader(ACCESS_CONTROL_ALLOW_ORIGIN_HEADER, refererHost)
                    .AddHeader(ACCESS_CONTROL_ALLOW_METHODS_HEADER, ACCESS_CONTROL_ALLOW_METHODS_VALUE)
                    .AddHeader(ACCESS_CONTROL_ALLOW_HEADERS_HEADER, ACCESS_CONTROL_ALLOW_HEADERS_VALUE)
                    .AddHeader(ACCESS_CONTROL_ALLOW_CREDENTIALS_HEADER, ACCESS_CONTROL_ALLOW_CREDENTIALS_VALUE)
                    .AddContentTypeHeader(NTurbo::EContentTypeHeader::PlainText)
                    .PushHttpResponse(ctx);
                return;
            }

            if (method != METHOD_GET) {
                NTurbo::MakeErrorHttpResponse(ctx, request.GetMethod() + TStringBuf(" is not allowed"));
                return;
            }

            if (!isCorsMatched) {
                NTurbo::THttpResponseBuilder httpResponse;
                httpResponse
                    .SetStatusCode(403)
                    .AddHeader(ACCESS_CONTROL_ALLOW_ORIGIN_HEADER, "")
                    .AddContentTypeHeader(NTurbo::EContentTypeHeader::PlainText)
                    .PushHttpResponse(ctx);
                return;
            }

            const auto& domainCgiRange = request.GetCgiParams().equal_range(DOMAIN_CGI_PARAMETER);
            if (domainCgiRange.first == domainCgiRange.second) {
                NTurbo::MakeErrorHttpResponse(ctx, TStringBuf("&domain= cgi is required"));
                return;
            }

            if (std::distance(domainCgiRange.first, domainCgiRange.second) != 1) {
                NTurbo::MakeErrorHttpResponse(ctx, TStringBuf("Only a single &domain= cgi is required"));
                return;
            }

            const auto& domainName = domainCgiRange.first->second;
            if (domainName.empty() || (domainName.length() > URL_MAX)) {
                NTurbo::MakeErrorHttpResponse(ctx, TStringBuf("Bad domain value"));
                return;
            }

            const TStringBuf sessionIdCookie = request.GetCookieValue(SESSION_ID_COOKIE);
            if (!sessionIdCookie.empty()){
                NAppHostHttp::THttpRequest blackboxHttpRequest;
                Y_ENSURE(NTurbo::NBlackbox::TryCreateSessionIdHttpRequest(
                    sessionIdCookie,
                    request.GetRealIp(),
                    request.GetHostname(),
                    NTurbo::NBlackbox::NAccountInfo::Nothing,
                    blackboxHttpRequest
                ));
                ctx.AddProtobufItem(std::move(blackboxHttpRequest), BLACKBOX_REQUEST_CONTEXT_TYPE);
                return;
            }

            NTurbo::THttpResponseBuilder responseBuilder;
            responseBuilder
                .AddHeader(ACCESS_CONTROL_ALLOW_ORIGIN_HEADER, refererHost)
                .AddHeader(ACCESS_CONTROL_ALLOW_HEADERS_HEADER, ACCESS_CONTROL_ALLOW_HEADERS_VALUE)
                .AddHeader(ACCESS_CONTROL_ALLOW_CREDENTIALS_HEADER, ACCESS_CONTROL_ALLOW_CREDENTIALS_VALUE)
                .AddContentTypeHeader(NTurbo::EContentTypeHeader::Json)
                .SetStatusCode(200);

            const TStringBuf yandexUidCookie = request.GetCookieValue(NTurbo::YANDEX_UID);
            if (!yandexUidCookie.empty()) {
                responseBuilder.SetContent(WrapTurboLoginInJson(GenerateUnauthorizedTurboLogin(domainName, yandexUidCookie, signatureSecretKey)));
            }
            else {
                ui64 reqTime = request.GetReqTime().Seconds();
                const TString newYandexUid = (
                    NTurbo::IsValidYandexUid(
                        request.GetYandexRandomUid(),
                        reqTime
                    )
                ) ? request.GetYandexRandomUid() : NTurbo::GenerateYandexUID(request.GetReqId(), reqTime);

                TStringBuf topLevelDomain = RU_YANDEX_TOP_LEVEL_DOMAIN;
                NTurbo::IsYandexHost(request.GetHostname(), &topLevelDomain);

                TStringStream setYandexUidCookieValue;
                setYandexUidCookieValue
                    << "yandexuid="
                    << newYandexUid
                    << "; path=/; domain=.yandex." << topLevelDomain
                    << "; expires=Thu, 31-Dec-2037 20:59:59 GMT";

                responseBuilder
                    .AddHeader(SET_COOKIE_HEADER_VALUE, setYandexUidCookieValue.Str())
                    .SetContent(WrapTurboLoginInJson(GenerateUnauthorizedTurboLogin(domainName, newYandexUid, signatureSecretKey)));
            }

            responseBuilder.PushHttpResponse(ctx);
        }
        catch (const std::exception& exp) {
            Y_DBGTRACE(VERBOSE, TString("Failed to process turbo login: ") + exp.what());
            NTurbo::MakeErrorHttpResponse(ctx, TStringBuf("Unexpected error"));
        }
    }

    void ProcessTurboLoginWithBlackbox(NAppHost::IServiceContext& ctx, const TVector<ui8>& signatureSecretKey) {
        try {
            bool isBlackBoxResponseFound = false;
            NAppHostHttp::THttpResponse httpResponse;

            for (const auto& contextName : {BLACKBOX_RESPONSE_CONTEXT_TYPE, HTTP_RESPONSE_CONTEXT_TYPE}) {
                const auto& blackboxHttpResponse = ctx.GetProtobufItemRefs(contextName);
                if (!blackboxHttpResponse.empty()) {
                    Y_ENSURE(blackboxHttpResponse.begin()->Fill(&httpResponse));
                    isBlackBoxResponseFound = true;
                    break;
                }
            }

            if (!isBlackBoxResponseFound) {
                return;
            }

            NTurbo::TRequest request{ctx};
            const auto& domainCgiRange = request.GetCgiParams().equal_range(DOMAIN_CGI_PARAMETER);
            Y_ENSURE(domainCgiRange.first != domainCgiRange.second);

            const auto& domainName = domainCgiRange.first->second;

            TString resultLogin;
            TString passportUid;
            if (!NTurbo::NBlackbox::TryParseSessionIdHttpResponse(httpResponse, passportUid) || (passportUid.empty())) {
                const TStringBuf yandexUidCookie = request.GetCookieValue(NTurbo::YANDEX_UID);
                if (yandexUidCookie.empty()) {
                    NTurbo::MakeErrorHttpResponse(ctx, TStringBuf("No yandexuid cookie"));
                    return;
                }
                resultLogin = GenerateUnauthorizedTurboLogin(domainName, yandexUidCookie, signatureSecretKey);
            }
            else {
                resultLogin = GenerateAuthorizedTurboLogin(domainName, passportUid, signatureSecretKey);
            }

            Y_ENSURE(!resultLogin.empty());

            TStringBuf refererHost;
            TStringBuf refererPath;
            SplitUrlToHostAndPath(request.GetReferer(), refererHost, refererPath);

            NTurbo::THttpResponseBuilder responseBuilder;
            responseBuilder
                .AddHeader(ACCESS_CONTROL_ALLOW_ORIGIN_HEADER, refererHost)
                .AddHeader(ACCESS_CONTROL_ALLOW_HEADERS_HEADER, ACCESS_CONTROL_ALLOW_HEADERS_VALUE)
                .AddHeader(ACCESS_CONTROL_ALLOW_CREDENTIALS_HEADER, ACCESS_CONTROL_ALLOW_CREDENTIALS_VALUE)
                .AddContentTypeHeader(NTurbo::EContentTypeHeader::Json)
                .SetContent(WrapTurboLoginInJson(resultLogin))
                .SetStatusCode(200)
                .PushHttpResponse(ctx);
        }
        catch (const std::exception& exp) {
            Y_DBGTRACE(VERBOSE, TString("Failed to process turbo login: ") + exp.what());
            NTurbo::MakeErrorHttpResponse(ctx, TStringBuf("Unexpected error"));
        }
    }
} // namespace NTurboLogin
