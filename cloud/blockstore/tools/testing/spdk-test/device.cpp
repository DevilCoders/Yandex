#include "device.h"

#include "options.h"

#include <cloud/blockstore/libs/spdk/env.h>

namespace NCloud::NBlockStore {

using namespace NSpdk;

////////////////////////////////////////////////////////////////////////////////

static const TString IP4_ANY = "0.0.0.0";
static const TString IP6_ANY = "[::]";

static const TString IQN_ANY = "ANY";
static const TString IQN_CLIENT = "iqn.2016-06.io.spdk:client";

static const TString NETMASK_ANY = "ANY";

static const TString DEVICE_FILE = "file";
static const TString DEVICE_NVME = "nvme";
static const TString DEVICE_SCSI = "scsi";

////////////////////////////////////////////////////////////////////////////////

TVector<TString> RegisterDevices(ISpdkEnvPtr env, const TOptions& opts)
{
    TVector<TString> devices;

    if (opts.NullDeviceName) {
        auto future = env->RegisterNullDevice(
            opts.NullDeviceName,
            opts.NullDeviceBlocksCount,
            opts.NullDeviceBlockSize);

        devices.push_back(future.GetValue(WaitTimeout));
    }

    if (opts.MemoryDeviceName) {
        auto future = env->RegisterMemoryDevice(
            opts.MemoryDeviceName,
            opts.MemoryDeviceBlocksCount,
            opts.MemoryDeviceBlockSize);

        devices.push_back(future.GetValue(WaitTimeout));
    }

    if (opts.FileDevicePath) {
        auto future = env->RegisterFileDevice(
            opts.FileDeviceName ? opts.FileDeviceName : DEVICE_FILE,
            opts.FileDevicePath,
            opts.FileDeviceBlockSize);

        devices.push_back(future.GetValue(WaitTimeout));
    }

    if (opts.NvmeDeviceTransportId) {
        auto transportId = opts.NvmeDeviceTransportId;
        if (opts.NvmeDeviceNqn) {
            transportId += " subnqn:" + opts.NvmeDeviceNqn;
        }

        auto future = env->RegisterNVMeDevices(
            opts.NvmeDeviceName ? opts.NvmeDeviceName : DEVICE_NVME,
            transportId);

        auto nvme = future.GetValue(WaitTimeout);

        devices.insert(devices.end(), nvme.begin(), nvme.end());
    }

    if (opts.ScsiDeviceUrl) {
        auto future = env->RegisterSCSIDevice(
            opts.ScsiDeviceName ? opts.ScsiDeviceName : DEVICE_SCSI,
            opts.ScsiDeviceUrl,
            opts.ScsiInitiatorIqn ? opts.ScsiInitiatorIqn : IQN_CLIENT);

        devices.push_back(future.GetValue(WaitTimeout));
    }

    if (opts.WrapDevice) {
        TVector<TString> wrappers;
        for (const auto& deviceName: devices) {
            auto device = env->OpenDevice(deviceName, true).GetValue(WaitTimeout);
            auto stats = env->QueryDeviceStats(deviceName).GetValue(WaitTimeout);

            auto future = env->RegisterDeviceWrapper(
                device,
                deviceName + "_wrapper",
                stats.BlocksCount,
                stats.BlockSize);

            wrappers.emplace_back(future.GetValue(WaitTimeout));
        }

        devices.swap(wrappers);
    }

    return devices;
}

ISpdkTargetPtr CreateTarget(
    ISpdkEnvPtr env,
    const TOptions& opts,
    const TVector<TString>& devices)
{
    if (opts.NvmeTargetTransportId) {
        env->AddTransport(opts.NvmeTargetTransportId).GetValue(WaitTimeout);
        env->StartListen(opts.NvmeTargetTransportId).GetValue(WaitTimeout);

        auto future = env->CreateNVMeTarget(
            opts.NvmeTargetNqn,
            devices,
            { opts.NvmeTargetTransportId });

        return future.GetValue(WaitTimeout);
    }

    if (opts.ScsiTargetPort) {
        auto portal = ISpdkEnv::TPortal(IP4_ANY, opts.ScsiTargetPort);
        auto initiator = ISpdkEnv::TInitiator(
            opts.ScsiInitiatorIqn ? opts.ScsiInitiatorIqn : IQN_ANY,
            NETMASK_ANY);

        env->CreatePortalGroup(1, { portal }).GetValue(WaitTimeout);
        env->CreateInitiatorGroup(1, { initiator }).GetValue(WaitTimeout);

        TVector<ISpdkEnv::TDevice> deviceLuns;
        int lun = 0;
        for (const auto& name: devices) {
            deviceLuns.emplace_back(name, lun++);
        }

        TVector<ISpdkEnv::TGroupMapping> groups = {
            { 1, 1 },
        };

        auto future = env->CreateSCSITarget(
            opts.ScsiTargetName ? opts.ScsiTargetName : DEVICE_SCSI,
            deviceLuns,
            groups);

        return future.GetValue(WaitTimeout);
    }

    return nullptr;
}

}   // namespace NCloud::NBlockStore
