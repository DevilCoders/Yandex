#pragma once

#include "public.h"

#include <cloud/blockstore/config/disk.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TDiskAgentConfig
{
private:
    NProto::TDiskAgentConfig Config;
    TString Rack;

public:
    TDiskAgentConfig() = default;

    TDiskAgentConfig(
            const NProto::TDiskAgentConfig& config,
            TString rack)
        : Config(config)
        , Rack(std::move(rack))
    {}

    bool GetEnabled() const;
    TString GetAgentId() const;
    ui64 GetSeqNumber() const;
    bool GetDedicatedDiskAgent() const;

    // TODO

    auto GetMemoryDevices() const
    {
        return Config.GetMemoryDevices();
    }

    auto GetFileDevices() const
    {
        return Config.GetFileDevices();
    }

    auto GetNVMeDevices() const
    {
        return Config.GetNvmeDevices();
    }

    auto GetNVMeTarget()
    {
        return Config.GetNvmeTarget();
    }

    auto GetRdmaTarget()
    {
        return Config.GetRdmaTarget();
    }

    auto HasRdmaTarget()
    {
        return Config.HasRdmaTarget();
    }

    ui32 GetPageSize() const;
    ui32 GetMaxPageCount() const;
    ui32 GetPageDropSize() const;

    TDuration GetRegisterRetryTimeout() const;
    TDuration GetSecureEraseTimeout() const;

    NProto::EDiskAgentBackendType GetBackend() const;
    NProto::EDeviceEraseMethod GetDeviceEraseMethod() const;

    bool GetAcquireRequired() const;

    TDuration GetReleaseInactiveSessionsTimeout() const;

    const TString& GetRack() const
    {
        return Rack;
    }

    bool GetDirectIoFlagDisabled() const;

    void Dump(IOutputStream& out) const;
    void DumpHtml(IOutputStream& out) const;
};

}   // namespace NCloud::NBlockStore::NStorage
