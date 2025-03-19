#pragma once

#include "public.h"

#include <cloud/storage/core/libs/diagnostics/solomon_counters.h>

namespace NCloud::NFileStore {

namespace NProto {

////////////////////////////////////////////////////////////////////////////////

class TFileSystem;
class TFileSystemStats;

}  // namespace NProto

////////////////////////////////////////////////////////////////////////////////

class TSimpleCounter
{
private:
    NMonitoring::TDynamicCounters::TCounterPtr TotalCounter;
    NMonitoring::TDynamicCounters::TCounterPtr FsCounter;

public:
    TSimpleCounter() = default;
    ~TSimpleCounter();

    void Register(
        NMonitoring::TDynamicCountersPtr totalCounters,
        NMonitoring::TDynamicCountersPtr fsCounters,
        const TString& name);

    void Add(i64 value);

    void Inc();
    void Dec();

    void Set(i64 value);
};

////////////////////////////////////////////////////////////////////////////////

class TFileSystemStatCounters
{
private:
    const ui32 BlockSize;

    TSimpleCounter SessionCount;
    TSimpleCounter MixedBytesCount;
    TSimpleCounter UsedBytesCount;
    TSimpleCounter FreshBytesCount;
    TSimpleCounter GarbageQueueSize;

public:
    TFileSystemStatCounters(
        const NProto::TFileSystem& fileSystem,
        IStorageCountersPtr storageCounters);

    void Update(
        const NProto::TFileSystemStats& stats);
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace NCloud::NFileStore
