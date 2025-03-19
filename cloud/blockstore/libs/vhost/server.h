#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/rdma/rdma.h>
#include <cloud/blockstore/libs/service/public.h>
#include <cloud/storage/core/libs/common/affinity.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/startable.h>
#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>

#include <library/cpp/threading/future/future.h>

namespace NCloud::NBlockStore::NVhost {

////////////////////////////////////////////////////////////////////////////////

struct TStorageOptions
{
    TString DeviceName;
    TString DiskId;
    TString ClientId;
    ui32 BlockSize = 0;
    ui64 BlocksCount = 0;
    ui32 VhostQueuesCount = 0;
    bool UnalignedRequestsDisabled = false;
};

////////////////////////////////////////////////////////////////////////////////

struct IServer
    : public IStartable
    , public IIncompleteRequestProvider
{
    virtual NThreading::TFuture<NProto::TError> StartEndpoint(
        TString socketPath,
        IStoragePtr storage,
        const TStorageOptions& options) = 0;

    virtual NThreading::TFuture<NProto::TError> StopEndpoint(
        const TString& socketPath) = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TServerConfig
{
    size_t ThreadsCount = 1;
    TAffinity Affinity;
};

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IVhostQueueFactoryPtr vhostQueueFactory,
    IDeviceHandlerFactoryPtr deviceHandlerFactory,
    const TServerConfig& serverConfig,
    NRdma::TRdmaHandler rdma);

}   // namespace NCloud::NBlockStore::NVhost
