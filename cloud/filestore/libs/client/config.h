#pragma once

#include "public.h"

#include <cloud/filestore/config/client.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NClient {

////////////////////////////////////////////////////////////////////////////////

class TClientConfig
{
private:
    const NProto::TClientConfig ProtoConfig;

public:
    TClientConfig(const NProto::TClientConfig& protoConfig)
        : ProtoConfig(protoConfig)
    {}

    TString GetHost() const;
    ui32 GetPort() const;

    ui32 GetThreadsCount() const;
    TDuration GetRequestTimeout() const;

    TDuration GetRetryTimeout() const;
    TDuration GetRetryTimeoutIncrement() const;
    TDuration GetConnectionErrorMaxRetryTimeout() const;

    void Dump(IOutputStream& out) const;
    void DumpHtml(IOutputStream& out) const;
};

////////////////////////////////////////////////////////////////////////////////

class TSessionConfig
{
private:
    const NProto::TSessionConfig ProtoConfig;

public:
    TSessionConfig(const NProto::TSessionConfig& protoConfig)
        : ProtoConfig(protoConfig)
    {}

    TString GetFileSystemId() const;
    TString GetClientId() const;

    TDuration GetSessionPingTimeout() const;
    TDuration GetSessionRetryTimeout() const;

    void Dump(IOutputStream& out) const;
    void DumpHtml(IOutputStream& out) const;

};

}   // namespace NCloud::NFileStore::NClient
