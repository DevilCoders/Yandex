#pragma once

#include "addr_list.h"
#include "blocker.h"
#include "request_params.h"
#include "stat.h"

#include <antirobot/lib/stats_writer.h>

#include <util/string/join.h>

#include <utility>

namespace NAntiRobot {
    class TCbbBannedIpHolder {
    public:
        using TServiceToFlags = std::array<TVector<TCbbGroupId>, HOST_NUMTYPES>;
        using TServiceToAddrSet = std::array<TRefreshableAddrSet, HOST_NUMTYPES>;

        enum class ECounter {
            Total     /* "banned_by_cbb_total" */,
            IpBased   /* "banned_by_cbb_ip_based" */,
            ByRegexp  /* "banned_by_cbb_regexp" */,
            Count
        };

        enum class EExpBinCounter {

            Count
        };

    public:
        TCbbBannedIpHolder() = default;

        TCbbBannedIpHolder(
            TCbbGroupId ipFlag,
            const TServiceToFlags& farmableFlags,
            const TRefreshableAddrSet& forIpIdentifications,
            const TServiceToAddrSet& forFarmableIdentifications
        )
            : IpBasedBanReason(CbbReason(ipFlag))
            , ForIpIdentifications(forIpIdentifications)
            , ForFarmableIdentifications(forFarmableIdentifications)
        {
            for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
                FarmableBanReasons[i] = CbbReason(farmableFlags[i]);
            }
        }

        TString GetBanReason(const TRequest& request) const {
            if (BannedByIpBasedIdentification(request)) {
                return IpBasedBanReason;
            }

            if (BannedByFarmableIdentification(request)) {
                return FarmableBanReasons[request.HostType];
            }

            return {};
        }

        void UpdateStats(const TRequest& request) {
            if (!request.CanShowCaptcha()) {
                return;
            }

            Counters.Inc(request, ECounter::Total);

            if (BannedByIpBasedIdentification(request)) {
                Counters.Inc(request, ECounter::IpBased);
            }
        }

        void PrintStats(TStatsWriter& out) const {
            Counters.Print(out);
        }

    public:
        TCategorizedStats<
            std::atomic<size_t>, ECounter,
            EHostType
        > Counters;

    private:
        bool BannedByIpBasedIdentification(const TRequest& request) const {
            return
                request.Uid.IpBased() &&
                ForIpIdentifications->Get()->ContainsActual(request.UserAddr);
        }

        bool BannedByFarmableIdentification(const TRequest& request) const {
            return
                request.Uid.Farmable() &&
                ForFarmableIdentifications[request.HostType]->Get()
                    ->ContainsActual(request.UserAddr);
        }

    private:
        TString IpBasedBanReason;
        std::array<TString, HOST_NUMTYPES> FarmableBanReasons;

        TRefreshableAddrSet ForIpIdentifications;
        TServiceToAddrSet ForFarmableIdentifications;
    };

}
