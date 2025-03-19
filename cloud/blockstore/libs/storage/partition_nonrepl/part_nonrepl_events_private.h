#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TEvNonreplPartitionPrivate
{
    //
    // RangeMigrated
    //

    struct TRangeMigrated
    {
        TBlockRange64 Range;
        TInstant ReadStartTs;
        TDuration ReadDuration;
        TInstant WriteStartTs;
        TDuration WriteDuration;
        TVector<IProfileLog::TBlockInfo> AffectedBlockInfos;

        TRangeMigrated(
                TBlockRange64 range,
                TInstant readStartTs,
                TDuration readDuration,
                TInstant writeStartTs,
                TDuration writeDuration,
                TVector<IProfileLog::TBlockInfo> affectedBlockInfos)
            : Range(std::move(range))
            , ReadStartTs(readStartTs)
            , ReadDuration(readDuration)
            , WriteStartTs(writeStartTs)
            , WriteDuration(writeDuration)
            , AffectedBlockInfos(std::move(affectedBlockInfos))
        {
        }
    };

    //
    // MigrateNextRange
    //

    struct TMigrateNextRange
    {
    };

    //
    // WriteOrZeroCompleted
    //

    struct TWriteOrZeroCompleted
    {
        ui64 RequestCounter;

        TWriteOrZeroCompleted(ui64 requestCounter)
            : RequestCounter(requestCounter)
        {
        }
    };

    //
    // OperationCompleted
    //

    struct TOperationCompleted
    {
        NProto::TPartitionStats Stats;

        ui64 TotalCycles = 0;
        ui64 ExecCycles = 0;

        TStackVec<int, 2> DeviceIndices;

        bool Failed = false;
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStorePrivateEvents::PARTITION_NONREPL_START,

        EvUpdateCounters,
        EvReadBlocksCompleted,
        EvWriteBlocksCompleted,
        EvZeroBlocksCompleted,
        EvRangeMigrated,
        EvMigrateNextRange,
        EvWriteOrZeroCompleted,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStorePrivateEvents::PARTITION_NONREPL_END,
        "EvEnd expected to be < TBlockStorePrivateEvents::PARTITION_NONREPL_END");

    using TEvUpdateCounters = TResponseEvent<TEmpty, EvUpdateCounters>;
    using TEvReadBlocksCompleted = TResponseEvent<TOperationCompleted, EvReadBlocksCompleted>;
    using TEvWriteBlocksCompleted = TResponseEvent<TOperationCompleted, EvWriteBlocksCompleted>;
    using TEvZeroBlocksCompleted = TResponseEvent<TOperationCompleted, EvZeroBlocksCompleted>;

    using TEvRangeMigrated = TResponseEvent<
        TRangeMigrated,
        EvRangeMigrated
    >;

    using TEvMigrateNextRange = TResponseEvent<
        TMigrateNextRange,
        EvMigrateNextRange
    >;

    using TEvWriteOrZeroCompleted = TResponseEvent<
        TWriteOrZeroCompleted,
        EvWriteOrZeroCompleted
    >;
};

}   // namespace NCloud::NBlockStore::NStorage
