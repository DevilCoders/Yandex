#include "part_actor.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <util/generic/vector.h>
#include <util/generic/xrange.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleHttpInfo_ForceCompaction(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    using namespace NMonitoringUtils;

    auto reply = [=] (
        const TActorContext& ctx,
        TRequestInfo& requestInfo,
        const TString& message)
    {
        TStringStream out;
        BuildTabletNotifyPageWithRedirect(out, message, TabletID());

        auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
        NCloud::Reply(ctx, requestInfo, std::move(response));
    };

    if (State->IsForcedCompactionRunning()) {
        reply(ctx, *requestInfo, "Compaction is already running");
        return;
    }

    ui64 blockIndex = 0;
    if (const auto& param = params.Get("BlockIndex")) {
        TryFromString(param, blockIndex);
    }

    ui32 blocksCount = 0;
    if (const auto& param = params.Get("BlocksCount")) {
        TryFromString(param, blocksCount);
    }

    auto& compactionMap = State->GetCompactionMap();

    TVector<ui32> rangesToCompact;
    if (blockIndex || blocksCount) {
        auto startIndex = Min(State->GetBlocksCount(), blockIndex);
        auto endIndex = Min(State->GetBlocksCount(), blockIndex + blocksCount);

        rangesToCompact = TVector<ui32>(
            ::xrange(
                compactionMap.GetRangeStart(startIndex),
                compactionMap.GetRangeStart(endIndex) + compactionMap.GetRangeSize(),
                compactionMap.GetRangeSize())
        );
    } else {
        rangesToCompact = compactionMap.GetNonEmptyRanges();
    }

    if (!rangesToCompact) {
        reply(ctx, *requestInfo, "Nothing to compact");
        return;
    }

    AddForcedCompaction(ctx, std::move(rangesToCompact), "partition-monitoring-compaction");
    EnqueueForcedCompaction(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
