#include "volume_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/storage/api/metering.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <cloud/storage/core/libs/common/media.h>

#include <util/generic/guid.h>
#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleLogUsageStats(
    const TEvVolumePrivate::TEvLogUsageStats::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    if (State) {
        const auto& config = State->GetConfig();
        if (config.GetCloudId() && config.GetFolderId()) {
            DumpUsageStats(ctx, GetVolumeStatus());
        }
    }

    ctx.Schedule(
        Config->GetUsageStatsLogPeriod(),
        new TEvVolumePrivate::TEvLogUsageStats());
}

void TVolumeActor::DumpUsageStats(
    const TActorContext& ctx,
    TVolumeActor::EStatus status)
{
#define JSON_QUOTED(s)  ("\"" s "\"")

    auto cur = ctx.Now().Seconds();
    auto prev = UsageIntervalBegin.Seconds();
    auto deltaSeconds = cur - prev;

    if (deltaSeconds > 0) {
        const auto& volumeConfig = State->GetMeta().GetVolumeConfig();

        auto blocksCount = GetBlocksCount();
        auto blockSize = volumeConfig.GetBlockSize();

        TStringStream out;
        out << '{'
            << JSON_QUOTED("schema") << ':' << JSON_QUOTED("nbs.volume.allocated.v1") << ','
            << JSON_QUOTED("folder_id") << ':' << volumeConfig.GetFolderId().Quote() << ','
            << JSON_QUOTED("cloud_id") << ':' << volumeConfig.GetCloudId().Quote() << ','
            << JSON_QUOTED("resource_id") << ':' << volumeConfig.GetDiskId().Quote() << ','
            << JSON_QUOTED("source_wt") << ':' << cur << ','
            << JSON_QUOTED("source_id") << ':' << FQDNHostName().Quote() << ','
            << JSON_QUOTED("id") << ':' << CreateGuidAsString().Quote() << ','
            << JSON_QUOTED("usage") << ':'
            << '{'
            << JSON_QUOTED("type") << ':' << JSON_QUOTED("delta") << ','
            << JSON_QUOTED("quantity") << ':' << deltaSeconds << ','
            << JSON_QUOTED("unit") << ':' << JSON_QUOTED("seconds") << ','
            << JSON_QUOTED("start") << ':' << prev << ','
            << JSON_QUOTED("finish") << ':' << cur
            << "},"
            << JSON_QUOTED("tags") << ':'
            << '{'
            << JSON_QUOTED("size") << ':' << (blocksCount * blockSize) << ','
            << JSON_QUOTED("type") << ':' << MediaKindToComputeType(
                (NCloud::NProto::EStorageMediaKind) volumeConfig.GetStorageMediaKind()).Quote() << ','
            << JSON_QUOTED("state") << ':' << GetVolumeStatusString(status).Quote()
            << '}'
            << "}\n";

        SendMeteringJson(ctx, out.Str());
        UsageIntervalBegin = ctx.Now();
    }

#undef JSON_QUOTED
}

}   // namespace NCloud::NBlockStore::NStorage
