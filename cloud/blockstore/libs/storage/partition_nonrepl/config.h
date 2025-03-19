#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/block_range.h>

#include <cloud/blockstore/libs/storage/protos/disk.pb.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/helpers.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/utility.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

using TDevices = google::protobuf::RepeatedPtrField<NProto::TDeviceConfig>;

struct TDeviceRequest
{
    const NProto::TDeviceConfig& Device;
    const ui32 DeviceIdx;
    const TBlockRange64 BlockRange;
    const TBlockRange64 DeviceBlockRange;

    TDeviceRequest(
            const NProto::TDeviceConfig& device,
            const ui32 deviceIdx,
            const TBlockRange64& blockRange,
            const TBlockRange64& deviceBlockRange)
        : Device(device)
        , DeviceIdx(deviceIdx)
        , BlockRange(blockRange)
        , DeviceBlockRange(deviceBlockRange)
    {
    }
};

class TNonreplicatedPartitionConfig
{
private:
    TDevices Devices;
    NProto::EVolumeIOMode IOMode;
    TString Name;
    ui32 BlockSize;
    NActors::TActorId ParentActorId;
    bool MuteIOErrors;
    bool MarkBlocksUsed;
    THashSet<TString> FreshDeviceIds;

    TVector<ui64> BlockIndices;

public:
    TNonreplicatedPartitionConfig(
            TDevices devices,
            NProto::EVolumeIOMode ioMode,
            TString name,
            ui32 blockSize,
            NActors::TActorId parentActorId,
            bool muteIOErrors,
            bool markBlocksUsed,
            THashSet<TString> freshDeviceIds)
        : Devices(std::move(devices))
        , IOMode(ioMode)
        , Name(std::move(name))
        , BlockSize(blockSize)
        , ParentActorId(std::move(parentActorId))
        , MuteIOErrors(muteIOErrors)
        , MarkBlocksUsed(markBlocksUsed)
        , FreshDeviceIds(std::move(freshDeviceIds))
    {
        Y_VERIFY(Devices.size());

        ui64 blockIndex = 0;
        for (const auto& device: Devices) {
            BlockIndices.push_back(blockIndex);
            blockIndex += device.GetBlocksCount();
        }
    }

public:
    TNonreplicatedPartitionConfigPtr Fork(TDevices devices) const
    {
        THashSet<TString> freshDeviceIds;
        for (const auto& device: devices) {
            if (FreshDeviceIds.contains(device.GetDeviceUUID())) {
                freshDeviceIds.insert(device.GetDeviceUUID());
            }
        }

        return std::make_shared<TNonreplicatedPartitionConfig>(
            std::move(devices),
            IOMode,
            Name,
            BlockSize,
            ParentActorId,
            MuteIOErrors,
            false,  // markBlocksUsed
            std::move(freshDeviceIds)
        );
    }

    const auto& GetDevices() const
    {
        return Devices;
    }

    auto& AccessDevices()
    {
        return Devices;
    }

    bool IsReadOnly() const
    {
        return IOMode != NProto::VOLUME_IO_OK;
    }

    bool GetMuteIOErrors() const
    {
        return MuteIOErrors;
    }

    bool GetMarkBlocksUsed() const
    {
        return MarkBlocksUsed;
    }

    const auto& GetName() const
    {
        return Name;
    }

    ui64 GetBlockCount() const
    {
        return BlockIndices.back() + Devices.rbegin()->GetBlocksCount();
    }

    auto GetBlockSize() const
    {
        return BlockSize;
    }

    const auto& GetParentActorId() const
    {
        return ParentActorId;
    }

    const auto& GetFreshDeviceIds() const
    {
        return FreshDeviceIds;
    }

public:
    TVector<TDeviceRequest> ToDeviceRequests(const TBlockRange64 blockRange) const
    {
        TVector<TDeviceRequest> res;
        VisitDeviceRequests(
            blockRange,
            [&] (
                const ui32 i,
                const TBlockRange64 requestRange,
                const TBlockRange64 relativeRange)
            {
                if (Devices[i].GetDeviceUUID()) {
                    res.emplace_back(
                        Devices[i],
                        i,
                        requestRange,
                        relativeRange);
                }

                return false;
            });

        return res;
    }

    bool DevicesReadyForReading(const TBlockRange64 blockRange) const
    {
        return VisitDeviceRequests(
            blockRange,
            [&] (
                const ui32 i,
                const TBlockRange64 requestRange,
                const TBlockRange64 relativeRange)
            {
                Y_UNUSED(requestRange);
                Y_UNUSED(relativeRange);

                return !Devices[i].GetDeviceUUID()
                    || FreshDeviceIds.contains(Devices[i].GetDeviceUUID());
            });
    }

    NCloud::NProto::TError MakeIOError(TString message) const
    {
        ui32 flags = 0;
        ui32 code = E_IO;
        if (MuteIOErrors) {
            SetProtoFlag(flags, NCloud::NProto::EF_SILENT);
            // for legacy clients
            code = E_IO_SILENT;
        }

        return MakeError(code, std::move(message), flags);
    }

private:
    TBlockRange64 DeviceRange(ui32 i) const
    {
        return TBlockRange64::WithLength(
            BlockIndices[i],
            Devices[i].GetBlocksCount()
        );
    }

    using TDeviceRequestVisitor = std::function<bool(
        const ui32 deviceIndex,
        const TBlockRange64 requestRange,
        const TBlockRange64 relativeRange
    )>;

    bool VisitDeviceRequests(
        const TBlockRange64 blockRange,
        const TDeviceRequestVisitor& visitor) const
    {
        const auto f = UpperBound(
            BlockIndices.begin(),
            BlockIndices.end(),
            blockRange.Start
        );

        const auto l = UpperBound(
            BlockIndices.begin(),
            BlockIndices.end(),
            blockRange.End
        );

        const auto fi = std::distance(BlockIndices.begin(), f) - 1;
        const auto li = std::distance(BlockIndices.begin(), l) - 1;

        Y_VERIFY(fi < Devices.size());
        Y_VERIFY(li < Devices.size());

        for (ui32 i = fi; i <= li; ++i) {
            const auto subRange = DeviceRange(i).Intersect(blockRange);
            const auto interrupted = visitor(
                i,
                subRange,
                TBlockRange64(
                    subRange.Start - BlockIndices[i],
                    subRange.End - BlockIndices[i]
                ));

            if (interrupted) {
                return false;
            }
        }

        return true;
    }
};

}   // namespace NCloud::NBlockStore::NStorage
