#include "options.h"

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/serialized_enum.h>

namespace NCloud::NBlockStore {

using namespace NLastGetopt;

////////////////////////////////////////////////////////////////////////////////

void TOptions::Parse(int argc, char** argv)
{
    TOpts opts;
    opts.AddHelpOption();

    opts.AddLongOption("test")
        .RequiredArgument("{" + GetEnumAllNames<ETestMode>() + "}")
        .DefaultValue(ToString(ETestMode::Target))
        .Handler1T<TString>([this] (const auto& s) {
            TestMode = FromString<ETestMode>(s);
        });

    opts.AddLongOption("null-device")
        .RequiredArgument("STR")
        .StoreResult(&NullDeviceName);

    opts.AddLongOption("null-device-blocks-count")
        .RequiredArgument("NUM")
        .StoreResult(&NullDeviceBlocksCount);

    opts.AddLongOption("null-device-block-size")
        .RequiredArgument("NUM")
        .StoreResult(&NullDeviceBlockSize);

    opts.AddLongOption("memory-device")
        .RequiredArgument("STR")
        .StoreResult(&MemoryDeviceName);

    opts.AddLongOption("memory-device-blocks-count")
        .RequiredArgument("NUM")
        .StoreResult(&MemoryDeviceBlocksCount);

    opts.AddLongOption("memory-device-block-size")
        .RequiredArgument("NUM")
        .StoreResult(&MemoryDeviceBlockSize);

    opts.AddLongOption("file-device")
        .RequiredArgument("STR")
        .StoreResult(&FileDeviceName);

    opts.AddLongOption("file-device-path")
        .RequiredArgument("STR")
        .StoreResult(&FileDevicePath);

    opts.AddLongOption("file-device-block-size")
        .RequiredArgument("NUM")
        .StoreResult(&FileDeviceBlockSize);

    opts.AddLongOption("nvme-device")
        .RequiredArgument("STR")
        .StoreResult(&NvmeDeviceName);

    opts.AddLongOption("nvme-device-transport")
        .RequiredArgument("STR")
        .StoreResult(&NvmeDeviceTransportId);

    opts.AddLongOption("nvme-device-nqn")
        .RequiredArgument("STR")
        .StoreResult(&NvmeDeviceNqn);

    opts.AddLongOption("nvme-target-transport")
        .RequiredArgument("STR")
        .StoreResult(&NvmeTargetTransportId);

    opts.AddLongOption("nvme-target-nqn")
        .RequiredArgument("STR")
        .StoreResult(&NvmeTargetNqn);

    opts.AddLongOption("scsi-device")
        .RequiredArgument("STR")
        .StoreResult(&ScsiDeviceName);

    opts.AddLongOption("scsi-device-url")
        .RequiredArgument("STR")
        .StoreResult(&ScsiDeviceUrl);

    opts.AddLongOption("scsi-target")
        .RequiredArgument("STR")
        .StoreResult(&ScsiTargetName);

    opts.AddLongOption("scsi-target-port")
        .RequiredArgument("NUM")
        .StoreResult(&ScsiTargetPort);

    opts.AddLongOption("scsi-initiator-iqn")
        .RequiredArgument("STR")
        .StoreResult(&ScsiInitiatorIqn);

    opts.AddLongOption("wrap")
        .NoArgument()
        .StoreTrue(&WrapDevice);

    opts.AddLongOption("io-depth")
        .RequiredArgument("NUM")
        .DefaultValue(1)
        .StoreResult(&MaxIoDepth);

    opts.AddLongOption("min-request-size")
        .RequiredArgument("NUM")
        .StoreResult(&MinRequestSize);

    opts.AddLongOption("max-request-size")
        .RequiredArgument("NUM")
        .StoreResult(&MaxRequestSize);

    opts.AddLongOption("write-rate")
        .RequiredArgument("NUM")
        .DefaultValue(WriteRate)
        .StoreResult(&WriteRate);

    opts.AddLongOption("cpu-mask")
        .OptionalArgument("STR")
        .StoreResult(&CpuMask);

    opts.AddLongOption("iops-limit")
        .RequiredArgument("NUM")
        .StoreResult(&IopsLimit);

    opts.AddLongOption("bw-rw-limit", "bandwidth limit (in megabytes)")
        .RequiredArgument("NUM")
        .StoreResult(&BandwidthLimit);

    opts.AddLongOption("bw-r-limit", "read bandwidth limit (in megabytes)")
        .RequiredArgument("NUM")
        .StoreResult(&ReadBandwidthLimit);

    opts.AddLongOption("bw-w-limit", "write bandwidth limit (in megabytes)")
        .RequiredArgument("NUM")
        .StoreResult(&WriteBandwidthLimit);

    opts.AddLongOption("collect-histogram")
        .NoArgument()
        .StoreTrue(&CollectHistogram);

    ui32 seconds = 0;
    opts.AddLongOption("test-duration")
        .RequiredArgument("NUM")
        .StoreResult(&seconds);

    const auto& verbose = opts.AddLongOption("verbose")
        .OptionalArgument("STR")
        .StoreResult(&VerboseLevel);

    TOptsParseResultException res(&opts, argc, argv);

    TestDuration = TDuration::Seconds(seconds);
    WriteRate = std::min(100u, WriteRate);

    if (res.Has(&verbose) && !VerboseLevel) {
        VerboseLevel = "debug";
    }

    if (!MaxRequestSize) {
        MaxRequestSize = MinRequestSize;
    }

    if (!MinRequestSize) {
        MinRequestSize = MaxRequestSize;
    }

    Y_ENSURE(MinRequestSize <= MaxRequestSize);
}

}   // namespace NCloud::NBlockStore
