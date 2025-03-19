#pragma once

#include "public.h"

#include <cloud/filestore/libs/client/public.h>
#include <cloud/filestore/libs/diagnostics/public.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/public.h>
#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>

#include <library/cpp/threading/future/future.h>

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////

struct IFileSystemDriver
    : public IIncompleteRequestProvider
{
    virtual ~IFileSystemDriver() = default;

    virtual NThreading::TFuture<NProto::TError> StartAsync() = 0;
    virtual NThreading::TFuture<void> StopAsync() = 0;
    virtual NThreading::TFuture<void> SuspendAsync() = 0;
};

////////////////////////////////////////////////////////////////////////////////

IFileSystemDriverPtr CreateFileSystemDriver(
    TFuseConfigPtr config,
    ILoggingServicePtr logging,
    IRequestStatsRegistryPtr requestStats,
    NClient::ISessionPtr session,
    IFileSystemFactoryPtr fileSystemFactory);

}   // namespace NCloud::NFileStore::NFuse
