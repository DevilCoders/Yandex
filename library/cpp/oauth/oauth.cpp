#include "oauth.h"

#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/ssh_sign/sign.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/datetime/base.h>
#include <util/random/random.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/strip.h>
#include <util/system/user.h>


TOauthTokenSquareRP::TOauthTokenSquareRP(ui32 retriesNum)
    : RetriesNum_(retriesNum)
{}

TMaybe<TDuration> TOauthTokenSquareRP::GetNextDelay() {
    if (Current_ > RetriesNum_) {
        return Nothing();
    }
    size_t maxDelay = (Current_ + 2) * (Current_ + 2);
    ++Current_;
    return TDuration::Seconds(1u + RandomNumber(maxDelay));
}

void TOauthTokenSquareRP::Reset() {
    Current_ = 1;
}

TOauthTokenParams::TOauthTokenParams(const TStringBuf clientId, const TStringBuf clientSecret)
: ClientId(clientId)
, ClientSecret(clientSecret)
{
}

TOauthTokenParams& TOauthTokenParams::SetKeyPath(const TStringBuf keyPath) {
    KeyPath = keyPath;
    return *this;
}

TOauthTokenParams& TOauthTokenParams::SetLogin(const TStringBuf login) {
    Login = login;
    return *this;
}

TOauthTokenParams& TOauthTokenParams::SetPasswordAuth(const TStringBuf login, const TStringBuf password) {
    Login = login;
    Password = password;
    return *this;
}

static TString ProcessOauthTokenRequest(
    TStringBuf params,
    TOauthTokenConnectOpts connectOpts = TOauthTokenConnectOpts()
) {
    connectOpts.RetryPolicy->Reset();
    const TString url = "https://oauth.yandex-team.ru";
    std::exception_ptr lastError = nullptr;
    do {
        TString out;
        try {
            {
                TSimpleHttpClient cl(url, 443, connectOpts.SocketTimeout, connectOpts.ConnectTimeout);
                TStringOutput s(out);
                cl.DoPost("/token", params, &s);
            }
            NJson::TJsonValue response;
            NJson::ReadJsonTree(out, &response);
            const TString token = response["access_token"].GetString();
            if (!token.empty())
                return token;
            else
                ythrow TGetOauthTokenApiException() << out;
        } catch (const THttpRequestException& e) {
            if (e.GetStatusCode() == 400) {
                std::throw_with_nested(TGetOauthTokenApiException() << out);
            } else {
                lastError = std::make_exception_ptr(e);
            }
        }
        const auto delay = connectOpts.RetryPolicy->GetNextDelay();
        if (!delay.Defined()) {
            break;
        }
        Sleep(*delay);
    } while(true);

    if (lastError) {
        std::rethrow_exception(lastError);
    } else {
        // this should never happen
        return {};
    }
}

static TString GetOauthTokenByPassword(TStringBuf clientId, TStringBuf clientSecret, TStringBuf login, TStringBuf password) {
    Y_ENSURE(!clientId.empty() && !clientSecret.empty() && !login.empty() && !password.empty());
    const TString params = TString::Join("client_id=", clientId, "&client_secret=", clientSecret,
                                         "&grant_type=password&username=", login, "&password=", password);
    return ProcessOauthTokenRequest(params);
}

static TString Base64EncodeUrlValid(TStringBuf s) {
    TString result = Base64EncodeUrl(s);
    result = StripStringRight(result, EqualsStripAdapter(','));
    return result;
}

TString GetOauthToken(const TOauthTokenParams& params, TOauthTokenConnectOpts connectOpts) {
    if(params.Password) {
        return GetOauthTokenByPassword(params.ClientId, params.ClientSecret, *params.Login, *params.Password);
    }

    const TString cLogin = params.Login ? TString{*params.Login} : GetUsername();
    const TString ts = ToString(time(nullptr));
    const TString toSign = TString::Join(ts, params.ClientId, cLogin);
    const TString body = TString::Join("client_id=", params.ClientId, "&client_secret=", params.ClientSecret,
                                       "&grant_type=ssh_key&ts=", ts, "&login=", cLogin, "&ssh_sign=");

    TGetOauthTokenApiException apiException;
    apiException << "Failed to get OAuth token, reasons are listed below:" << Endl;

    const auto tryProcessOauthTokenRequest = [&](const NSS::TResult& result) -> TMaybe<TString> {
        if (const auto* err = std::get_if<NSS::TErrorMsg>(&result)) {
            apiException << *err << Endl;
            return {};
        }

        const NSS::TSignedData* signedData = std::get_if<NSS::TSignedData>(&result);
        Y_ASSERT(signedData);

        TString request = body + Base64EncodeUrlValid(signedData->Sign);
        if (signedData->Type == NSS::ESignType::CERT) {
            request += "&public_cert=" + Base64EncodeUrlValid(signedData->Key);
        }

        try {
            return ProcessOauthTokenRequest(request, connectOpts);
        } catch (const TGetOauthTokenApiException& e) {
            apiException << e.what() << Endl;
        }
        return {};
    };

    if (params.KeyPath) {
        NSS::TLazySignerPtr signer = NSS::SignByRsaKey(*params.KeyPath);

        std::optional<NSS::TResult> result;
        while ((result = signer->SignNext(toSign))) {
            if (const auto& t = tryProcessOauthTokenRequest(*result)) {
                return *t;
            }
        }
    } else {
        for (NSS::TLazySignerPtr& signer : NSS::SignByAny(cLogin)) {
            std::optional<NSS::TResult> result;

            while ((result = signer->SignNext(toSign))) {
                if (const auto& t = tryProcessOauthTokenRequest(*result)) {
                    return *t;
                }
            }
        }
    }

    ythrow apiException;
    Y_UNREACHABLE();
}
