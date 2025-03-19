#include "filesystem_counters.h"
#include "storage_counters.h"

#include <cloud/filestore/libs/storage/tablet/protos/tablet.pb.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

TSimpleCounter::~TSimpleCounter()
{
    if (TotalCounter && FsCounter) {
        TotalCounter->Sub(FsCounter->GetAtomic());
    }
}

void TSimpleCounter::Register(
    NMonitoring::TDynamicCountersPtr total,
    NMonitoring::TDynamicCountersPtr fs,
    const TString& name)
{
    TotalCounter = total->GetExpiringCounter(name);
    FsCounter = fs->GetExpiringCounter(name);
}

void TSimpleCounter::Add(i64 value)
{
    TotalCounter->Add(value);
    FsCounter->Add(value);
}

void TSimpleCounter::Inc()
{
    TotalCounter->Inc();
    FsCounter->Inc();
}

void TSimpleCounter::Dec()
{
    TotalCounter->Dec();
    FsCounter->Dec();
}

void TSimpleCounter::Set(i64 value)
{
    TotalCounter->Add(value - FsCounter->GetAtomic());
    FsCounter->Set(value);
}

////////////////////////////////////////////////////////////////////////////////

TFileSystemStatCounters::TFileSystemStatCounters(
        const NProto::TFileSystem& fileSystem,
        IStorageCountersPtr storageCounters)
    : BlockSize(fileSystem.GetBlockSize())
{
    auto fs = storageCounters->RegisterFileSystemCounters(
        fileSystem.GetFileSystemId());
    auto total = storageCounters->GetCounters(
        fileSystem.GetStorageMediaKind());

    SessionCount.Register(total, fs, "SessionCount");
    MixedBytesCount.Register(total, fs, "MixedBytesCount");
    FreshBytesCount.Register(total, fs, "FreshBytesCount");
    UsedBytesCount.Register(total, fs, "UsedBytesCount");
    GarbageQueueSize.Register(total, fs, "GarbageQueueSize");
}

void TFileSystemStatCounters::Update(const NProto::TFileSystemStats& stats)
{
    SessionCount.Set(stats.GetUsedSessionsCount());

    MixedBytesCount.Set(stats.GetMixedBlocksCount() * BlockSize);
    UsedBytesCount.Set(stats.GetUsedBlocksCount() * BlockSize);

    FreshBytesCount.Set(stats.GetFreshBytesCount());
    GarbageQueueSize.Set(stats.GetGarbageQueueSize());
}

}   // namespace NCloud::NFileStore
