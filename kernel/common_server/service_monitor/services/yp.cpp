#include "yp.h"

#include <yp/cpp/yp/client.h>
#include <util/system/env.h>
#include <library/cpp/logger/global/global.h>


void TYpApiConfig::Init(const TYandexConfig::Section* section) {
    ConnectionLiveTime = section->GetDirectives().Value<TDuration>("ConnectionLiveTime", ConnectionLiveTime);

    const TString YpToken = GetEnv("YP_TOKEN");
    VERIFY_WITH_LOG(!YpToken.empty(), "Environment variable YP_TOKEN is empty. There must be OAuth token to access YDeploy and YP services. See https://wiki.yandex-team.ru/yp/ .");
    auto children = section->GetAllChildren();
    for (auto&& [clusterName, section] : children) {
        YpApi[clusterName].Init(section, YpToken);
    }
}

void TYpApiConfig::ToString(IOutputStream& os) const {
    os << "ConnectionLiveTime: " << ConnectionLiveTime << Endl;
    for (auto&& [clusterName, clientConfig] : YpApi) {
        os << "<" << clusterName << ">" << Endl;
        clientConfig.ToString(os);
        os << "</" << clusterName << ">" << Endl;
    }
}


TYpServicesOperator::TYpServicesOperator(const TYpApiConfig& config)
    : Config(config)
    , ClientsCache(Config.GetConnectionLiveTime())
{
}

TAtomicSharedPtr<NYP::NClient::TClient> TYpServicesOperator::GetClient(const TString& dataCenter) const {
    auto it = Config.GetYpApi().find(dataCenter);
    if (it == Config.GetYpApi().end()) {
        return nullptr;
    }
    const TYpClientConfig& ypClientConfig = it->second;

    TAtomicSharedPtr<NYP::NClient::TClient> result;
    if (TMaybe<TAtomicSharedPtr<NYP::NClient::TClient>> maybeClient = ClientsCache.GetValue(dataCenter)) {
        result = std::move(maybeClient).GetRef();
        VERIFY_WITH_LOG(result, "Extracted empty maybeClient from ClientsCache for DataCenter \"%s\".", dataCenter.c_str());
    } else {
        result = MakeAtomicShared<NYP::NClient::TClient>(ypClientConfig);
        auto copy = result;
        ClientsCache.PutValue(dataCenter, std::move(copy));
    }

    return result;
}

TString TYpServicesOperator::GetAddress(const TString& dataCenter, const TString& defaultAddress) const {
    auto it = Config.GetYpApi().find(dataCenter);
    return (it != Config.GetYpApi().end() ? it->second.Address() : defaultAddress);
}

