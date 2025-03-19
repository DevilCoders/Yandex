#include "config.h"

#include <util/stream/file.h>

void TRequestConfig::Init(const TYandexConfig::Section* section, const TMap<TString, NSimpleMeta::TConfig>* requestPolicy) {
    TBase::Init(section, requestPolicy);
    RequestTimeout = section->GetDirectives().Value<TDuration>("RequestTimeout", RequestTimeout);
}

void TRequestConfig::ToString(IOutputStream& os) const {
    TBase::ToString(os);
    os << "RequestTimeout: " << RequestTimeout << Endl;
}

void TRequestConfig::Authorize(NNeh::THttpRequest& /* request */) const {
}

void TTokenAuthRequestConfig::Init(const TYandexConfig::Section* section, const TMap<TString, NSimpleMeta::TConfig>* requestPolicy) {
    TBase::Init(section, requestPolicy);
    TokenPath = section->GetDirectives().Value("TokenPath", TokenPath);
    if (!!TokenPath) {
        Token = Strip(TFileInput(TokenPath).ReadAll());
    } else {
        Token = section->GetDirectives().Value<TString>("Token", Token);
    }
}

void TTokenAuthRequestConfig::ToString(IOutputStream& os) const {
    TBase::ToString(os);
    os << "TokenPath: " << TokenPath << Endl;
}

void TTokenAuthRequestConfig::Authorize(NNeh::THttpRequest& request) const {
    request.AddHeader("Authorization", "OAuth " + GetToken());
}
