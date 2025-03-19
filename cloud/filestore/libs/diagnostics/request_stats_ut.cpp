#include "request_stats.h"

#include <cloud/storage/core/libs/common/timer.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NFileStore::NStorage {

using namespace NMonitoring;

struct TBootstrap
{
    TDynamicCountersPtr Counters;
    ITimerPtr Timer;
    IRequestStatsRegistryPtr Registry;

    TBootstrap()
        : Counters{MakeIntrusive<TDynamicCounters>()}
        , Timer{CreateWallClockTimer()}
        , Registry{CreateRequestStatsRegistry(nullptr, Counters, Timer)}
    {}
};

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TRequestStatRegistryTest)
{
    Y_UNIT_TEST(ShouldCreateRemoveStats)
    {
        TBootstrap bootstrap;

        auto componentCounters = bootstrap.Counters
                ->FindSubgroup("component", "server_fs");
        UNIT_ASSERT(componentCounters);
        componentCounters = componentCounters->FindSubgroup("host", "cluster");
        UNIT_ASSERT(componentCounters);

        const TString fs = "test";
        const TString client = "client";
        auto stats = bootstrap.Registry->GetFileSystemStats(
            fs,
            client,
            NProto::STORAGE_MEDIA_SSD);
        {
            auto fsCounters = componentCounters
                ->FindSubgroup("filesystem", fs);
            UNIT_ASSERT(fsCounters);

            auto clientCounters = fsCounters->FindSubgroup("client", client);
            UNIT_ASSERT(clientCounters);
            UNIT_ASSERT(clientCounters->FindSubgroup("type", "ssd"));
        }

        bootstrap.Registry->Unregister(fs, client);
        {
            auto fsCounters = componentCounters
                ->FindSubgroup("filesystem", fs);
            UNIT_ASSERT(fsCounters);
            UNIT_ASSERT(!fsCounters->FindSubgroup("client", client));
        }
    }
}

} // namespace NCloud::NFileStore::NStorage