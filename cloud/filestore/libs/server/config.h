#pragma once

#include "public.h"

#include <cloud/filestore/config/server.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NServer {

////////////////////////////////////////////////////////////////////////////////

class TServerConfig
{
private:
    const NProto::TServerConfig ProtoConfig;

public:
    TServerConfig(const NProto::TServerConfig& protoConfig = {})
        : ProtoConfig(protoConfig)
    {}

    TString GetHost() const;
    ui32 GetPort() const;

    ui32 GetMaxMessageSize() const;
    ui32 GetMemoryQuotaBytes() const;
    ui32 GetPreparedRequestsCount() const;

    ui32 GetThreadsCount() const;
    ui32 GetGrpcThreadsLimit() const;

    bool GetKeepAliveEnabled() const;
    TDuration GetKeepAliveIdleTimeout() const;
    TDuration GetKeepAliveProbeTimeout() const;
    ui32 GetKeepAliveProbesCount() const;

    TDuration GetShutdownTimeout() const;

    void Dump(IOutputStream& out) const;
    void DumpHtml(IOutputStream& out) const;
};

}   // namespace NCloud::NFileStore::NServer
