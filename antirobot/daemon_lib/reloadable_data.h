#pragma once

#include "entity_dictionary.h"
#include "geo_checker.h"
#include "request_classifier.h"
#include "request_group_classifier.h"
#include "trusted_users.h"

#include <antirobot/lib/bad_user_agents.h>
#include <antirobot/lib/ip_list.h>
#include <antirobot/lib/ip_list_pid.h>
#include <antirobot/lib/ip_map.h>
#include <antirobot/lib/mini_geobase.h>
#include <antirobot/lib/search_crawlers.h>

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/noncopyable.h>

namespace NLCookie {
    struct IKeychain;
}

namespace NAntiRobot {
    struct TReloadableData : public TNonCopyable {
        TBadUserAgents BadUserAgents;
        TBadUserAgents BadUserAgentsNew;
        NThreading::TRcuAccessor<TIpListProjId> TurboProxyIps;
        NThreading::TRcuAccessor<TIpListProjId> SpecialIps;
        std::array<NThreading::TRcuAccessor<TIpListProjId>, HOST_NUMTYPES> ServiceToYandexIps;
        NThreading::TRcuAccessor<TIpListProjId> UaProxyIps;
        std::array<NThreading::TRcuAccessor<TIpListProjId>, HOST_NUMTYPES> ServiceToWhitelist;
        NThreading::TRcuAccessor<TIpRangeMap<ECrawler>> SearchEngines;
        NThreading::TRcuAccessor<TIpListProjId> PrivilegedIps;
        NThreading::TRcuAccessor<TGeoChecker> GeoChecker;
        NThreading::TRcuAccessor<THashDictionary> FraudJa3;
        NThreading::TRcuAccessor<THashDictionary> KinopoiskFilmsHoneypots;
        NThreading::TRcuAccessor<THashDictionary> KinopoiskNamesHoneypots;
        NThreading::TRcuAccessor<TSubnetDictionary<24, 64>> FraudSubnet;
        NThreading::TRcuAccessor<TSubnetDictionary<24, 32>> FraudSubnetNew;
        NThreading::TRcuAccessor<TMiniGeobase> MiniGeobase;
        NThreading::TRcuAccessor<TTrustedUsers> TrustedUsers;
        NThreading::TRcuAccessor<NLCookie::IKeychain*> LKeychain;
        NThreading::TRcuAccessor<TRequestClassifier> RequestClassifier;
        NThreading::TRcuAccessor<TMarketJwsStatsDictionary> MarketJwsStatesStats;
        NThreading::TRcuAccessor<TMarketStatsDictionary> MarketJa3Stats;
        NThreading::TRcuAccessor<TMarketStatsDictionary> MarketSubnetStats;
        NThreading::TRcuAccessor<TMarketStatsDictionary> MarketUAStats;
        NThreading::TRcuAccessor<THashDictionary> AutoruJa3;
        NThreading::TRcuAccessor<TSubnetDictionary<24, 32>> AutoruSubnet;
        NThreading::TRcuAccessor<TEntityDict<float>> AutoruUA;

        TReloadableData();
    };
} // namespace NAntiRobot
