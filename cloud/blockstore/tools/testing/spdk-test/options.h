#pragma once

#include "private.h"

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/ylimits.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

enum class ETestMode
{
    Target,
    Initiator
};

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    ETestMode TestMode = ETestMode::Target;

    TString NullDeviceName;
    ui64 NullDeviceBlocksCount = 1024;
    ui32 NullDeviceBlockSize = 4*1024;

    TString MemoryDeviceName;
    ui64 MemoryDeviceBlocksCount = 1024;
    ui32 MemoryDeviceBlockSize = 4*1024;

    TString FileDeviceName;
    TString FileDevicePath;
    ui32 FileDeviceBlockSize = 4*1024;

    TString NvmeDeviceName;
    TString NvmeDeviceTransportId;
    TString NvmeDeviceNqn;

    TString NvmeTargetTransportId;
    TString NvmeTargetNqn;

    TString ScsiDeviceName;
    TString ScsiDeviceUrl;

    TString ScsiTargetName;
    TString ScsiTargetHost;
    ui32 ScsiTargetPort = 0;

    TString ScsiInitiatorIqn;

    bool WrapDevice = false;

    ui32 MaxIoDepth = 1;
    ui32 MinRequestSize = 0;
    ui32 MaxRequestSize = 0;
    ui32 WriteRate = 80;

    ui64 IopsLimit = Max();
    ui64 BandwidthLimit = Max();
    ui64 ReadBandwidthLimit = Max();
    ui64 WriteBandwidthLimit = Max();

    bool CollectHistogram = false;

    TDuration TestDuration;

    TString CpuMask;

    TString VerboseLevel;

    void Parse(int argc, char** argv);
};

}   // namespace NCloud::NBlockStore
