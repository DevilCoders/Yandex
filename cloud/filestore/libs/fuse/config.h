#pragma once

#include "config.h"

#include <cloud/filestore/config/filesystem.pb.h>
#include <cloud/filestore/config/fuse.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////

struct TFuseConfig
{
private:
    const NProto::TFuseConfig ProtoConfig;

public:
    TFuseConfig(const NProto::TFuseConfig& protoConfig)
        : ProtoConfig(protoConfig)
    {}

    TString GetFileSystemId() const;
    TString GetClientId() const;

    TString GetSocketPath() const;
    TString GetMountPath() const;
    bool GetReadOnly() const;
    bool GetDebug() const;

    TDuration GetLockRetryTimeout() const;

    ui32 GetFuseMaxWritePages() const;

    ui64 GetMountSeqNumber() const;
    ui32 GetVhostQueuesCount() const;

    ui32 GetXAttrCacheSize() const;
    TDuration GetXAttrCacheTimeout() const;

    void Dump(IOutputStream& out) const;
    void DumpHtml(IOutputStream& out) const;
};

////////////////////////////////////////////////////////////////////////////////

struct TFileSystemConfig
{
private:
    const NProto::TFileSystemConfig ProtoConfig;

public:
    TFileSystemConfig(const NProto::TFileSystemConfig& protoConfig)
        : ProtoConfig(protoConfig)
    {}

    TString GetFileSystemId() const;
    ui32 GetBlockSize() const;

    TDuration GetLockRetryTimeout() const;
    TDuration GetEntryTimeout() const;
    TDuration GetAttrTimeout() const;

    ui32 GetXAttrCacheLimit() const;
    TDuration GetXAttrCacheTimeout() const;

    ui32 GetMaxListBufferSize() const;

    void Dump(IOutputStream& out) const;
    void DumpHtml(IOutputStream& out) const;
};

}   // namespace NCloud::NFileStore::NFuse
