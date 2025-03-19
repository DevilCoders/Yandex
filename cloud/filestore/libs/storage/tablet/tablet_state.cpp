#include "tablet_state_impl.h"

#include <cloud/filestore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/filestore/libs/storage/tablet/model/block.h>

#include <library/cpp/protobuf/json/proto2json.h>

#include <util/string/builder.h>

namespace NCloud::NFileStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

IBlockLocation2RangeIndexPtr CreateHasher(const NProto::TFileSystem& fs)
{
    auto hasher = CreateRangeIdHasher(fs.GetRangeIdHasherType());
    Y_VERIFY(
        hasher,
        "unsupported hasher type: %u",
        fs.GetRangeIdHasherType());

    return hasher;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void InitProfileLogByteRanges(
    const ui32 blockSize,
    const TVector<T>& blobs,
    NProto::TProfileLogRequestInfo& request)
{
    for (const auto& blob: blobs) {
        ui64 lastNodeId = NProto::E_INVALID_NODE_ID;
        ui64 lastBlockIndex = 0;
        ui64 blockCount = 0;
        for (const auto& block: blob.Blocks) {
            if (block.BlockIndex == lastBlockIndex + blockCount
                    && block.NodeId == lastNodeId)
            {
                ++blockCount;
            } else {
                if (lastNodeId != NProto::E_INVALID_NODE_ID) {
                    auto* range = request.AddRanges();
                    range->SetNodeId(lastNodeId);
                    range->SetOffset(lastBlockIndex * blockSize);
                    range->SetBytes(blockCount * blockSize);
                }

                lastNodeId = block.NodeId;
                lastBlockIndex = block.BlockIndex;
                blockCount = 1;
            }
        }

        if (lastNodeId != NProto::E_INVALID_NODE_ID) {
            auto* range = request.AddRanges();
            range->SetNodeId(lastNodeId);
            range->SetOffset(lastBlockIndex * blockSize);
            range->SetBytes(blockCount * blockSize);
        }
    }
}

template void InitProfileLogByteRanges(
    const ui32 blockSize,
    const TVector<TMixedBlob>& blobs,
    NProto::TProfileLogRequestInfo& request);

template void InitProfileLogByteRanges(
    const ui32 blockSize,
    const TVector<TMixedBlobMeta>& blobs,
    NProto::TProfileLogRequestInfo& request);

////////////////////////////////////////////////////////////////////////////////

TIndexTabletState::TIndexTabletState()
    : Impl(new TImpl())
{}

TIndexTabletState::~TIndexTabletState()
{}

void TIndexTabletState::LoadState(
    ui32 generation,
    const NProto::TFileSystem& fileSystem,
    const NProto::TFileSystemStats& fileSystemStats,
    const NCloud::NProto::TTabletStorageInfo& tabletStorageInfo)
{
    Generation = generation;
    LastStep = 0;
    LastCollectCounter = 0;

    FileSystem.CopyFrom(fileSystem);
    FileSystemStats.CopyFrom(fileSystemStats);
    TabletStorageInfo.CopyFrom(tabletStorageInfo);

    if (FileSystemStats.GetLastNodeId() < RootNodeId) {
        FileSystemStats.SetLastNodeId(RootNodeId);
    }

    LoadChannels();

    Impl->RangeIdHasher = CreateHasher(fileSystem);
}

void TIndexTabletState::UpdateConfig(
    TIndexTabletDatabase& db,
    const NProto::TFileSystem& fileSystem)
{
    FileSystem.CopyFrom(fileSystem);
    db.WriteFileSystem(fileSystem);

    UpdateChannels();

    Impl->RangeIdHasher = CreateHasher(fileSystem);
}

void TIndexTabletState::DumpStats(IOutputStream& os) const
{
    NProtobufJson::TProto2JsonConfig config;
    config.SetFormatOutput(true);

    NProtobufJson::Proto2Json(
        FileSystemStats,
        os,
        config
    );
}

}   // namespace NCloud::NFileStore::NStorage
