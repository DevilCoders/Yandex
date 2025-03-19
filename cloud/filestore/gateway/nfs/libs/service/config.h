#pragma once

#include "config.h"

#include <cloud/filestore/config/nfs_gateway.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

class TFileStoreServiceConfig
{
private:
    const NProto::TNfsGatewayConfig ProtoConfig;

public:
    TFileStoreServiceConfig(const NProto::TNfsGatewayConfig& protoConfig)
        : ProtoConfig(protoConfig)
    {}

    TDuration GetRequestTimeout() const;

    TDuration GetSessionRetryTimeout() const;
    TDuration GetSessionPingTimeout() const;

    void Dump(IOutputStream& out) const;
    void DumpHtml(IOutputStream& out) const;
};

}   // namespace NCloud::NFileStore::NGateway
