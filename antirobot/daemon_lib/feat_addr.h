#pragma once

#include "config.h"
#include "config_global.h"
#include "reloadable_data.h"

#include <antirobot/lib/addr.h>

#include <kernel/geo/utils.h>

namespace NAntiRobot {
    class TFeaturedAddr : public TAddr {
        bool IsYandex_ = false;
        bool IsPrivileged_ = false;
        bool IsWhitelisted_ = false;
        bool IsEuropean_ = false;
        bool IsYandexFromGeoBase = false;
        bool IsTor_ = false;
        bool IsProxy_ = false;
        bool IsVpn_ = false;
        bool IsHosting_ = false;
        bool IsMobile_ = false;
        ECrawler SearchEngine_ = ECrawler::None;

        TGeoRegion GeoRegion_ = -1;
        int32_t CountryId_ = -1;

        TMiniGeobase::TMask MiniGeobaseMask_;
    public:
        TFeaturedAddr() = default;

        TFeaturedAddr(
                const TAddr& addr,
                const TReloadableData& data,
                EHostType service,
                bool disableIsYandex,
                bool disablePrivileged,
                bool disableWhitelist)
            : TAddr(addr)
        {
            if (addr.Valid()) {
                IsYandex_ = !disableIsYandex && ANTIROBOT_DAEMON_CONFIG.IgnoreYandexIps && data.ServiceToYandexIps[service].Get()->Contains(addr);
                IsPrivileged_ = !disablePrivileged && data.PrivilegedIps.Get()->Contains(addr);
                IsWhitelisted_ = !disableWhitelist && data.ServiceToWhitelist[service].Get()->Contains(addr);
                if (const auto seData = data.SearchEngines.Get()->Find(addr); seData.Defined()) {
                    SearchEngine_ = *seData;
                }

                const auto geoChecker = data.GeoChecker.Get();
                if (const auto traits = geoChecker->GetTraits(addr)) {
                    GeoRegion_ = traits->RegionId < 1 ? NGeobase::EARTH_REGION_ID : traits->RegionId;
                    IsEuropean_ = geoChecker->IsEuropean(GeoRegion_);
                    IsYandexFromGeoBase = traits->IsYandexNet() || traits->IsYandexStaff() || traits->IsYandexTurbo();
                    IsTor_ = traits->IsTor();
                    IsProxy_ = traits->IsProxy();
                    IsVpn_ = traits->IsVpn();
                    IsHosting_ = traits->IsHosting();
                    IsMobile_ = traits->IsMobile();
                }

                CountryId_ = geoChecker->GetCountryId(GeoRegion_);
                MiniGeobaseMask_ = data.MiniGeobase.Get()->GetValue(addr);
            }
        }

        bool IsYandex() const {
            return IsYandex_;
        }

        bool IsPrivileged() const {
            return IsYandex() || IsPrivileged_;
        }

        bool IsWhitelisted() const {
            return IsPrivileged() || IsWhitelisted_;
        }

        bool IsSearchEngine() const {
            return SearchEngine_ != ECrawler::None;
        }

        ECrawler SearchEngine() const {
            return SearchEngine_;
        }

        bool IsEuropean() const {
            return IsEuropean_;
        }

        bool IsYandexNet() const {
            return IsYandexFromGeoBase;
        }

        bool IsTor() const {
            return IsTor_;
        }

        bool IsProxy() const {
            return IsProxy_;
        }

        bool IsVpn() const {
            return IsVpn_;
        }

        bool IsHosting() const {
            return IsHosting_;
        }

        bool IsMobile() const {
            return IsMobile_;
        }

        bool IsMikrotik() const {
            return MiniGeobaseMask_.IsMikrotik();
        }

        bool IsSquid() const {
            return MiniGeobaseMask_.IsSquid();
        }

        bool IsDdoser() const {
            return MiniGeobaseMask_.IsDdoser();
        }

        TGeoRegion GeoRegion() const {
            return GeoRegion_;
        }

        int32_t CountryId() const {
            return CountryId_;
        }

        const TVector<TString> IpList() const {
            TVector<TString> result;
            if (IsWhitelisted_) {
                result.push_back("whitelist");
            }
            if (IsPrivileged_) {
                result.push_back("privileged");
            }
            if (IsYandex_) {
                result.push_back("yandex");
            }
            if (IsSearchEngine()) {
                TStringStream ss;
                ss << "crawler:" << SearchEngine();
                result.push_back(ss.Str());
            }
            return result;
        }

        ui64 MiniGeobaseMask() const {
            return MiniGeobaseMask_.AsUint64();
        }
    };
}

template <>
inline void Out<NAntiRobot::TFeaturedAddr>(IOutputStream& out, const NAntiRobot::TFeaturedAddr& addr) {
    Out<NAntiRobot::TAddr>(out, addr);
}
