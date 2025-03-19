#pragma once

#include <kernel/common_server/library/cache/cache_with_live_time.h>
#include <kernel/common_server/util/accessor.h>
#include <yp/cpp/yp/request_model.h>
#include <library/cpp/yconf/conf.h>


class TYpClientConfig: public NYP::NClient::TClientOptions {
public:
    using TBase = NYP::NClient::TClientOptions;

    void Init(const TYandexConfig::Section* section, const TString& ypToken) {
        SetAddress(section->GetDirectives().Value("Address", Address()));
        SetEnableSsl(section->GetDirectives().Value("EnableSSL", EnableSsl()));
        SetTimeout(section->GetDirectives().Value("Timeout", Timeout()));
        SetToken(ypToken);
        SetReadOnlyMode(true);
    }

    void ToString(IOutputStream& os) const {
        os << "Address: " << Address() << Endl;
        os << "EnableSSL: " << EnableSsl() << Endl;
        os << "Timeout: " << Timeout() << Endl;
    }
};


class TYpApiConfig {
    using TClusterToConfig = TMap<TString, TYpClientConfig>;
public:
    CSA_DEFAULT(TYpApiConfig, TClusterToConfig, YpApi);
    CS_ACCESS(TYpApiConfig, TDuration, ConnectionLiveTime, TDuration::Minutes(1));
public:
    void Init(const TYandexConfig::Section* section);
    void ToString(IOutputStream& os) const;
};


class TYpServicesOperator {
public:
    TYpServicesOperator(const TYpApiConfig& config);

    TAtomicSharedPtr<NYP::NClient::TClient> GetClient(const TString& dataCenter) const;
    TString GetAddress(const TString& dataCenter, const TString& defaultAddress = {}) const;

private:
    TYpApiConfig Config;

    mutable TCacheWithLiveTime<TString, TAtomicSharedPtr<NYP::NClient::TClient>> ClientsCache;
};
