#include "mon_page_wrapper.h"
#include "user_stats_actor.h"

#include <cloud/blockstore/libs/diagnostics/volume_stats.h>

#include <cloud/storage/core/libs/kikimr/helpers.h>

#include <ydb/core/base/appdata.h>

#include <util/generic/fwd.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/monlib/dynamic_counters/encode.h>
#include <library/cpp/monlib/service/pages/templates.h>
#include <library/cpp/monlib/encode/json/json.h>
#include <library/cpp/monlib/encode/spack/spack_v1.h>
#include <library/cpp/monlib/encode/text/text.h>

namespace NCloud::NBlockStore::NStorage::NUserStats {

////////////////////////////////////////////////////////////////////////////////

TUserStatsActor::TUserStatsActor(TVector<IUserMetricsSupplierPtr> providers)
    : Providers(std::move(providers))
{}

void TUserStatsActor::Bootstrap(const NActors::TActorContext& ctx)
{
    Become(&TThis::StateWork);
    RegisterPages(ctx);
}

void TUserStatsActor::RegisterPages(const NActors::TActorContext& ctx)
{
    auto mon = NKikimr::AppData(ctx)->Mon;
    if (mon) {
        auto* rootPage = mon->RegisterIndexPage("blockstore", "BlockStore");

        mon->RegisterActorPage(rootPage, "user_stats", "UserStats",
            true, ctx.ExecutorThread.ActorSystem, SelfId());

        mon->Register(new TMonPageWrapper(
            "blockstore/user_stats/json",
            [this] (IOutputStream& out) {
                return OutputJsonPage(out);
            }));
        mon->Register(new TMonPageWrapper(
            "blockstore/user_stats/spack",
            [this] (IOutputStream& out) {
                return OutputSpackPage(out);
            }));
    }
}

void TUserStatsActor::RenderHtmlInfo(IOutputStream& out) const
{
    auto encoder = NMonitoring::EncoderText(&out);

    encoder->OnStreamBegin();
    {
        TReadGuard g{Lock};

        for (auto&& provider : Providers) {
            provider->Append(TInstant::Zero(), encoder.Get());
        }
    }
    encoder->OnStreamEnd();
}

void TUserStatsActor::OutputJsonPage(IOutputStream& out) const
{
    out << NMonitoring::HTTPOKJSON;
    auto encoder = NMonitoring::EncoderJson(&out);

    encoder->OnStreamBegin();
    {
        TReadGuard g{Lock};

        for (auto&& provider : Providers) {
            provider->Append(TInstant::Zero(), encoder.Get());
        }
    }
    encoder->OnStreamEnd();
}

void TUserStatsActor::OutputSpackPage(IOutputStream& out) const
{
    out << NMonitoring::HTTPOKSPACK;

    auto encoder = NMonitoring::EncoderSpackV1(
        &out,
        NMonitoring::ETimePrecision::SECONDS,
        NMonitoring::ECompression::IDENTITY);

    encoder->OnStreamBegin();
    {
        TReadGuard g{Lock};

        for (auto&& provider : Providers) {
            provider->Append(TInstant::Now(), encoder.Get());
        }
    }
    encoder->OnStreamEnd();
}

////////////////////////////////////////////////////////////////////////////////

void TUserStatsActor::HandleHttpInfo(
    const NActors::NMon::TEvHttpInfo::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    TStringStream out;
    RenderHtmlInfo(out);

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NActors::NMon::TEvHttpInfoRes>(out.Str()));
}

void TUserStatsActor::HandleUserStatsProviderCreate(
    const TEvUserStats::TEvUserStatsProviderCreate::TPtr& ev,
    const NActors::TActorContext&)
{
    TEvUserStats::TUserStatsProviderCreate* msg = ev->Get();

    if (msg->Provider) {
        TReadGuard g{Lock};

        Providers.push_back(msg->Provider);
    }
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TUserStatsActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(NActors::NMon::TEvHttpInfo, HandleHttpInfo);

        HFunc(TEvUserStats::TEvUserStatsProviderCreate, HandleUserStatsProviderCreate);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::USER_STATS);
    }
}

}   // NCloud::NBlockStore::NStorage::NUserStats
