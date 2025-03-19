#include "storage_counters.h"

#include <cloud/storage/core/libs/diagnostics/solomon_counters.h>

namespace NCloud::NFileStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TStorageCounters final
    : public IStorageCounters
{
private:
    NMonitoring::TDynamicCountersPtr RootCounter;
    NMonitoring::TDynamicCountersPtr FsCountersRoot;
    NMonitoring::TDynamicCountersPtr TotalCountersRoot;
    NMonitoring::TDynamicCountersPtr SsdCounters;
    NMonitoring::TDynamicCountersPtr HddCounters;

public:
    TStorageCounters(NMonitoring::TDynamicCountersPtr root)
        : RootCounter(std::move(root))
        , FsCountersRoot(
                FILESTORE_COUNTERS_ROOT(RootCounter)
                    ->GetSubgroup("component", "storage_fs")
                    ->GetSubgroup("host", "cluster"))
        , TotalCountersRoot(
                FILESTORE_COUNTERS_ROOT(RootCounter)
                    ->GetSubgroup("component", "storage"))
        , SsdCounters(TotalCountersRoot->GetSubgroup("type", "ssd"))
        , HddCounters(TotalCountersRoot->GetSubgroup("type", "hdd"))
    {}

    NMonitoring::TDynamicCountersPtr RegisterFileSystemCounters(
        const TString& fsId) override;

    NMonitoring::TDynamicCountersPtr GetCounters(
        NCloud::NProto::EStorageMediaKind mediaKind) override;
};

NMonitoring::TDynamicCountersPtr TStorageCounters::RegisterFileSystemCounters(
    const TString& fsId)
{
    return FsCountersRoot->GetSubgroup("filesystem", fsId);
}

NMonitoring::TDynamicCountersPtr TStorageCounters::GetCounters(
    NCloud::NProto::EStorageMediaKind mediaKind)
{
    switch (mediaKind) {
        case NCloud::NProto::STORAGE_MEDIA_SSD: return SsdCounters;
        case NCloud::NProto::STORAGE_MEDIA_DEFAULT:
        case NCloud::NProto::STORAGE_MEDIA_HYBRID:
        case NCloud::NProto::STORAGE_MEDIA_HDD: return HddCounters;
        default: {
            Y_FAIL("unsupported media kind: %u", static_cast<ui32>(mediaKind));
        }
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IStorageCountersPtr CreateStorageCounters(
    NMonitoring::TDynamicCountersPtr root)
{
    return std::make_shared<TStorageCounters>(std::move(root));
}

}   // namespace NCloud::NFileStore
