#pragma once

#include "async_impl.h"

#include <kernel/common_server/util/accessor.h>

class TRequestConfig : public TAsyncApiImpl::TConfig {
    using TBase = TAsyncApiImpl::TConfig;

    RTLINE_READONLY_ACCEPTOR_DEF(RequestTimeout, TDuration);

public:
    TRequestConfig(const TDuration requestTimeout = TDuration::Seconds(1))
        : TBase()
        , RequestTimeout(requestTimeout)
    {
    }

    void Init(const TYandexConfig::Section* section, const TMap<TString, NSimpleMeta::TConfig>* requestPolicy);
    void ToString(IOutputStream& os) const;
    void Authorize(NNeh::THttpRequest& /* request */) const;
};

class TTokenAuthRequestConfig : public TRequestConfig {
    using TBase = TRequestConfig;

    RTLINE_READONLY_ACCEPTOR(Token, TString, "");
    RTLINE_READONLY_ACCEPTOR(TokenPath, TString, "");

public:
    TTokenAuthRequestConfig(const TDuration requestTimeout = TDuration::Seconds(1))
        : TBase(requestTimeout)
    {
    }

    void Init(const TYandexConfig::Section* section, const TMap<TString, NSimpleMeta::TConfig>* requestPolicy);
    void ToString(IOutputStream& os) const;
    void Authorize(NNeh::THttpRequest& request) const;
};
