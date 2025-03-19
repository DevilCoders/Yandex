#pragma once

#include "public.h"

#include <cloud/storage/core/protos/media.pb.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

struct IStorageCounters
{
    virtual ~IStorageCounters() = default;

    virtual NMonitoring::TDynamicCountersPtr RegisterFileSystemCounters(
        const TString& id) = 0;

    virtual NMonitoring::TDynamicCountersPtr GetCounters(
        NCloud::NProto::EStorageMediaKind mediaKind) = 0;
};

////////////////////////////////////////////////////////////////////////////////

IStorageCountersPtr CreateStorageCounters(
    NMonitoring::TDynamicCountersPtr root);

}   // namespace NCloud::NFileStore
