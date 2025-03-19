#include "disk_agent.h"
#include "disk_agent_actor.h"

#include "testlib/test_env.h"

#include <cloud/blockstore/libs/common/block_checksum.h>
#include <cloud/blockstore/libs/common/iovector.h>

#include <library/cpp/lwtrace/all.h>
#include <library/cpp/protobuf/util/pb_io.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NServer;
using namespace NThreading;

using namespace NDiskAgentTest;

namespace {

////////////////////////////////////////////////////////////////////////////////

const TDuration WaitTimeout = TDuration::Seconds(5);

////////////////////////////////////////////////////////////////////////////////

NLWTrace::TQuery QueryFromString(const TString& text)
{
    TStringInput in(text);

    NLWTrace::TQuery query;
    ParseFromTextFormat(in, query);
    return query;
}

TFsPath TryGetRamDrivePath()
{
    auto p = GetRamDrivePath();
    return !p
        ? GetSystemTempDir()
        : p;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDiskAgentTest)
{
    Y_UNIT_TEST(ShouldRegisterDevicesOnStartup)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig()
                | WithBackend(NProto::DISK_AGENT_BACKEND_SPDK)
                | WithMemoryDevices({
                    MemoryDevice("MemoryDevice1") | WithPool("memory"),
                    MemoryDevice("MemoryDevice2")
                })
                | WithFileDevices({
                    FileDevice("FileDevice3"),
                    FileDevice("FileDevice4") | WithPool("local-ssd")
                })
                | WithNVMeDevices({
                    NVMeDevice("nvme1", {"NVMeDevice5"}) | WithPool("nvme"),
                    NVMeDevice("nvme2", {"NVMeDevice6"})
                }))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const auto& devices = env.DiskRegistryState->Devices;
        UNIT_ASSERT_VALUES_EQUAL(6, devices.size());

        // common properties
        for (auto& [uuid, device]: devices) {
            UNIT_ASSERT_VALUES_EQUAL(uuid, device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(uuid, device.GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("the-rack", device.GetRack());
        }

        {
            auto& device = devices.at("MemoryDevice1");
            UNIT_ASSERT_VALUES_EQUAL("", device.GetTransportId());
            UNIT_ASSERT_VALUES_EQUAL("memory", device.GetPoolName());
            UNIT_ASSERT_VALUES_EQUAL(DefaultDeviceBlockSize, device.GetBlockSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlocksCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL("agent:", device.GetBaseName());
        }

        {
            auto& device = devices.at("MemoryDevice2");
            UNIT_ASSERT_VALUES_EQUAL("", device.GetTransportId());
            UNIT_ASSERT_VALUES_EQUAL("", device.GetPoolName());
            UNIT_ASSERT_VALUES_EQUAL(DefaultDeviceBlockSize, device.GetBlockSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlocksCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL("agent:", device.GetBaseName());
        }

        {
            auto& device = devices.at("FileDevice3");
            UNIT_ASSERT_VALUES_EQUAL("", device.GetTransportId());
            UNIT_ASSERT_VALUES_EQUAL("", device.GetPoolName());
            UNIT_ASSERT_VALUES_EQUAL(DefaultDeviceBlockSize, device.GetBlockSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultStubBlocksCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL("agent:", device.GetBaseName());
        }

        {
            auto& device = devices.at("FileDevice4");
            UNIT_ASSERT_VALUES_EQUAL("", device.GetTransportId());
            UNIT_ASSERT_VALUES_EQUAL("local-ssd", device.GetPoolName());
            UNIT_ASSERT_VALUES_EQUAL(DefaultDeviceBlockSize, device.GetBlockSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultStubBlocksCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL("agent:", device.GetBaseName());
        }

        {
            auto& device = devices.at("NVMeDevice5");
            UNIT_ASSERT_VALUES_EQUAL("", device.GetTransportId());
            UNIT_ASSERT_VALUES_EQUAL("nvme", device.GetPoolName());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, device.GetBlockSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultStubBlocksCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL("agent:nvme1:", device.GetBaseName());
        }

        {
            auto& device = devices.at("NVMeDevice6");
            UNIT_ASSERT_VALUES_EQUAL("", device.GetTransportId());
            UNIT_ASSERT_VALUES_EQUAL("", device.GetPoolName());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, device.GetBlockSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultStubBlocksCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL("agent:nvme2:", device.GetBaseName());
        }
    }

    Y_UNIT_TEST(ShouldAcquireAndReleaseDevices)
    {
        TTestBasicRuntime runtime;

        const TVector<TString> uuids {
            "MemoryDevice1",
            "MemoryDevice2",
            "MemoryDevice3"
        };

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig(uuids))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        diskAgent.AcquireDevices(
            uuids,
            "session-1",
            NProto::VOLUME_ACCESS_READ_WRITE
        );
        diskAgent.ReleaseDevices(uuids, "session-1");
    }

    Y_UNIT_TEST(ShouldAcquireWithRateLimits)
    {
        TTestBasicRuntime runtime;

        const TVector<TString> uuids {
            "MemoryDevice1",
            "MemoryDevice2",
            "MemoryDevice3"
        };

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig(uuids))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        NSpdk::TDeviceRateLimits limits {
            .IopsLimit = 1000
        };

        diskAgent.AcquireDevices(
            uuids,
            "session-1",
            NProto::VOLUME_ACCESS_READ_WRITE,
            0,
            limits
        );
        diskAgent.ReleaseDevices(uuids, "session-1");
    }

    Y_UNIT_TEST(ShouldAcquireDevicesOnlyOnce)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({"MemoryDevice1"}))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT(env.DiskRegistryState->Devices.contains("MemoryDevice1"));

        const TVector<TString> uuids{
            "MemoryDevice1"
        };

        {
            auto response = diskAgent.AcquireDevices(
                uuids,
                "session-id",
                NProto::VOLUME_ACCESS_READ_WRITE
            );
            UNIT_ASSERT(!HasError(response->Record));
        }

        {
            diskAgent.SendAcquireDevicesRequest(
                uuids,
                "session-id2",
                NProto::VOLUME_ACCESS_READ_WRITE
            );
            auto response = diskAgent.RecvAcquireDevicesResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_INVALID_SESSION);
            UNIT_ASSERT(response->GetErrorReason().Contains("already acquired"));
        }

        // reacquire with same session id is ok
        {
            auto response = diskAgent.AcquireDevices(
                uuids,
                "session-id",
                NProto::VOLUME_ACCESS_READ_WRITE
            );
            UNIT_ASSERT(!HasError(response->Record));
        }
    }

    Y_UNIT_TEST(ShouldAcquireDevicesWithMountSeqNumber)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({"MemoryDevice1"}))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids{
            "MemoryDevice1"
        };

        {
            auto response = diskAgent.AcquireDevices(
                uuids,
                "session-id",
                NProto::VOLUME_ACCESS_READ_WRITE
            );
            UNIT_ASSERT(!HasError(response->Record));
        }

        {
            diskAgent.SendAcquireDevicesRequest(
                uuids,
                "session-id2",
                NProto::VOLUME_ACCESS_READ_WRITE,
                1
            );
            auto response = diskAgent.RecvAcquireDevicesResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }

        {
            diskAgent.SendAcquireDevicesRequest(
                uuids,
                "session-id3",
                NProto::VOLUME_ACCESS_READ_WRITE,
                2
            );
            auto response = diskAgent.RecvAcquireDevicesResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }

        {
            diskAgent.SendAcquireDevicesRequest(
                uuids,
                "session-id4",
                NProto::VOLUME_ACCESS_READ_WRITE,
                2
            );
            auto response = diskAgent.RecvAcquireDevicesResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_INVALID_SESSION, response->GetStatus());
        }
    }

    Y_UNIT_TEST(ShouldAcquireDevicesOnlyOnceInGroup)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({"MemoryDevice1", "MemoryDevice2"}))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(env.DiskRegistryState->Devices.size(), 2);
        UNIT_ASSERT(env.DiskRegistryState->Devices.contains("MemoryDevice1"));
        UNIT_ASSERT(env.DiskRegistryState->Devices.contains("MemoryDevice2"));

        {
            const TVector<TString> uuids{
                "MemoryDevice1"
            };

            auto response = diskAgent.AcquireDevices(
                uuids,
                "session-id",
                NProto::VOLUME_ACCESS_READ_WRITE
            );
            UNIT_ASSERT(!HasError(response->Record));
        }

        {
            const TVector<TString> uuids{
                "MemoryDevice1",
                "MemoryDevice2"
            };

            diskAgent.SendAcquireDevicesRequest(
                uuids,
                "session-id2",
                NProto::VOLUME_ACCESS_READ_WRITE
            );
            auto response = diskAgent.RecvAcquireDevicesResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_INVALID_SESSION);
            UNIT_ASSERT(response->GetErrorReason().Contains("already acquired"));
            UNIT_ASSERT(response->GetErrorReason().Contains(
                "MemoryDevice1"
            ));
        }

        // reacquire with same session id is ok
        {
            const TVector<TString> uuids{
                "MemoryDevice1"
            };

            auto response = diskAgent.AcquireDevices(
                uuids,
                "session-id",
                NProto::VOLUME_ACCESS_READ_WRITE
            );
            UNIT_ASSERT(!HasError(response->Record));
        }
    }

    Y_UNIT_TEST(ShouldRejectAcquireNonexistentDevice)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({"MemoryDevice1"}))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids{
            "MemoryDevice1",
            "nonexistent uuid"
        };

        diskAgent.SendAcquireDevicesRequest(
            uuids,
            "session-id",
            NProto::VOLUME_ACCESS_READ_WRITE
        );
        auto response = diskAgent.RecvAcquireDevicesResponse();
        UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_NOT_FOUND);
        UNIT_ASSERT(response->GetErrorReason().Contains(uuids.back()));
    }

    Y_UNIT_TEST(ShouldPerformIo)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({
                "MemoryDevice1",
                "MemoryDevice2",
                "MemoryDevice3",
            }))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids{
            "MemoryDevice1",
            "MemoryDevice2"
        };

        const TString sessionId = "session-1";

        diskAgent.AcquireDevices(
            uuids,
            sessionId,
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        const auto blocksCount = 10;

        {
            const auto response = ReadDeviceBlocks(
                runtime, diskAgent, uuids[0], 0, blocksCount, sessionId);

            const auto& record = response->Record;

            UNIT_ASSERT(record.HasBlocks());

            const auto& iov = record.GetBlocks();
            UNIT_ASSERT_VALUES_EQUAL(iov.BuffersSize(), blocksCount);
            for (auto& buffer : iov.GetBuffers()) {
                UNIT_ASSERT_VALUES_EQUAL(buffer.size(), DefaultBlockSize);
            }
        }

        {
            TVector<TString> blocks;
            auto sglist = ResizeBlocks(
                blocks,
                blocksCount,
                TString(DefaultBlockSize, 'X'));

            WriteDeviceBlocks(runtime, diskAgent, uuids[0], 0, sglist, sessionId);
            ZeroDeviceBlocks(runtime, diskAgent, uuids[0], 0, 10, sessionId);
        }

        {
            auto request = std::make_unique<TEvDiskAgent::TEvWriteDeviceBlocksRequest>();
            request->Record.SetSessionId(sessionId);
            request->Record.SetDeviceUUID(uuids[0]);
            request->Record.SetStartIndex(0);
            request->Record.SetBlockSize(DefaultBlockSize);

            auto sgList = ResizeIOVector(*request->Record.MutableBlocks(), 1, 1_MB);

            for (auto& buffer: sgList) {
                memset(const_cast<char*>(buffer.Data()), 'Y', buffer.Size());
            }

            diskAgent.SendRequest(MakeDiskAgentServiceId(), std::move(request));
            runtime.DispatchEvents(NActors::TDispatchOptions());
            auto response = diskAgent.RecvWriteDeviceBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), S_OK);
        }
    }

    Y_UNIT_TEST(ShouldSupportReadOnlySessions)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({
                "MemoryDevice1",
                "MemoryDevice2",
                "MemoryDevice3",
            }))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids{
            "MemoryDevice1",
            "MemoryDevice2"
        };

        const TString writerSessionId = "writer-1";
        const TString readerSessionId1 = "reader-1";
        const TString readerSessionId2 = "reader-2";

        diskAgent.AcquireDevices(
            uuids,
            writerSessionId,
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        diskAgent.AcquireDevices(
            uuids,
            readerSessionId1,
            NProto::VOLUME_ACCESS_READ_ONLY
        );

        diskAgent.AcquireDevices(
            uuids,
            readerSessionId2,
            NProto::VOLUME_ACCESS_READ_ONLY
        );

        const auto blocksCount = 10;

        TVector<TString> blocks;
        auto sglist = ResizeBlocks(
            blocks,
            blocksCount,
            TString(DefaultBlockSize, 'X'));

        WriteDeviceBlocks(runtime, diskAgent, uuids[0], 0, sglist, writerSessionId);
        // diskAgent.ZeroDeviceBlocks(uuids[0], 0, blocksCount / 2, writerSessionId);

        for (auto sid: {readerSessionId1, readerSessionId2}) {
            diskAgent.SendWriteDeviceBlocksRequest(
                uuids[0],
                0,
                sglist,
                sid
            );

            auto response = diskAgent.RecvWriteDeviceBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_INVALID_SESSION);
        }

        for (auto sid: {readerSessionId1, readerSessionId2}) {
            diskAgent.SendZeroDeviceBlocksRequest(
                uuids[0],
                0,
                blocksCount / 2,
                sid
            );

            auto response = diskAgent.RecvZeroDeviceBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_INVALID_SESSION);
        }

        for (auto sid: {writerSessionId, readerSessionId1, readerSessionId2}) {
            const auto response = ReadDeviceBlocks(
                runtime,
                diskAgent,
                uuids[0],
                0,
                blocksCount,
                sid);

            const auto& record = response->Record;

            UNIT_ASSERT(record.HasBlocks());

            const auto& iov = record.GetBlocks();
            UNIT_ASSERT_VALUES_EQUAL(iov.BuffersSize(), blocksCount);

            for (auto& buffer : iov.GetBuffers()) {
                UNIT_ASSERT_VALUES_EQUAL(buffer.size(), DefaultBlockSize);
            }

            /*
            ui32 i = 0;

            while (i < blocksCount / 2) {
                UNIT_ASSERT_VALUES_EQUAL(
                    TString(10, 0),
                    iov.GetBuffers(i).substr(0, 10)
                );

                ++i;
            }

            while (i < blocksCount) {
                UNIT_ASSERT_VALUES_EQUAL(
                    TString(10, 'X'),
                    iov.GetBuffers(i).substr(0, 10)
                );

                ++i;
            }
            */
        }
    }

    Y_UNIT_TEST(ShouldPerformIoUnsafe)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig(
                { "MemoryDevice1", "MemoryDevice2", "MemoryDevice3" },
                false))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids{
            "MemoryDevice1",
            "MemoryDevice2"
        };

        const auto blocksCount = 10;

        {
            const auto response = ReadDeviceBlocks(
                runtime, diskAgent, uuids[0], 0, blocksCount, "");

            const auto& record = response->Record;

            UNIT_ASSERT(record.HasBlocks());

            const auto& iov = record.GetBlocks();
            UNIT_ASSERT_VALUES_EQUAL(iov.BuffersSize(), blocksCount);
            for (auto& buffer : iov.GetBuffers()) {
                UNIT_ASSERT_VALUES_EQUAL(buffer.size(), DefaultBlockSize);
            }
        }

        TVector<TString> blocks;
        auto sglist = ResizeBlocks(
            blocks,
            blocksCount,
            TString(DefaultBlockSize, 'X'));

        WriteDeviceBlocks(runtime, diskAgent, uuids[0], 0, sglist, "");
        ZeroDeviceBlocks(runtime, diskAgent, uuids[0], 0, 10, "");
    }

    Y_UNIT_TEST(ShouldSecureEraseDevice)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({
                "MemoryDevice1",
                "MemoryDevice2",
                "MemoryDevice3",
            }))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        diskAgent.SecureEraseDevice("MemoryDevice1");
    }

    Y_UNIT_TEST(ShouldUpdateStats)
    {
        auto const workingDir = TryGetRamDrivePath();

        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With([&] {
                NProto::TDiskAgentConfig agentConfig;
                agentConfig.SetBackend(NProto::DISK_AGENT_BACKEND_AIO);
                agentConfig.SetAcquireRequired(true);
                agentConfig.SetEnabled(true);

                *agentConfig.AddFileDevices() = PrepareFileDevice(
                    workingDir / "error",
                    "dev1");

                auto* device = agentConfig.MutableMemoryDevices()->Add();
                device->SetName("dev2");
                device->SetBlocksCount(1024);
                device->SetBlockSize(DefaultBlockSize);
                device->SetDeviceId("dev2");

                return agentConfig;
            }())
            .Build();

        UNIT_ASSERT(env.DiskRegistryState->Stats.empty());

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        diskAgent.AcquireDevices(
            TVector<TString> {"dev1", "dev2"},
            "session-1",
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        auto waitForStats = [&] (auto cb) {
            runtime.AdvanceCurrentTime(TDuration::Seconds(15));

            TDispatchOptions options;
            options.FinalEvents = {
                TDispatchOptions::TFinalEventCondition(
                    TEvDiskRegistry::EvUpdateAgentStatsRequest)
            };

            runtime.DispatchEvents(options);

            const auto& stats = env.DiskRegistryState->Stats;
            UNIT_ASSERT_EQUAL(1, stats.size());

            auto agentStats = stats.begin()->second;
            SortBy(*agentStats.MutableDeviceStats(), [] (auto& x) {
                return x.GetDeviceUUID();
            });

            cb(agentStats);
        };

        auto read = [&] (const auto& uuid) {
            return ReadDeviceBlocks(runtime, diskAgent, uuid, 0, 1, "session-1")->GetStatus();
        };

        waitForStats([] (auto agentStats) {
            UNIT_ASSERT_EQUAL(2, agentStats.DeviceStatsSize());

            auto& dev0 = agentStats.GetDeviceStats(0);
            UNIT_ASSERT_VALUES_EQUAL("dev1", dev0.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, dev0.GetErrors());
            UNIT_ASSERT_VALUES_EQUAL(0, dev0.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(0, dev0.GetBytesRead());

            auto& dev1 = agentStats.GetDeviceStats(1);
            UNIT_ASSERT_VALUES_EQUAL("dev2", dev1.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, dev1.GetErrors());
            UNIT_ASSERT_VALUES_EQUAL(0, dev1.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(0, dev1.GetBytesRead());
        });

        UNIT_ASSERT_VALUES_EQUAL(E_IO, read("dev1"));
        UNIT_ASSERT_VALUES_EQUAL(E_IO, read("dev1"));
        UNIT_ASSERT_VALUES_EQUAL(S_OK, read("dev2"));
        UNIT_ASSERT_VALUES_EQUAL(S_OK, read("dev2"));

        waitForStats([] (auto agentStats) {
            UNIT_ASSERT_EQUAL(2, agentStats.DeviceStatsSize());

            auto& dev0 = agentStats.GetDeviceStats(0);
            UNIT_ASSERT_VALUES_EQUAL("dev1", dev0.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(2, dev0.GetErrors());
            UNIT_ASSERT_VALUES_EQUAL(2, dev0.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(0, dev0.GetBytesRead());

            auto& dev1 = agentStats.GetDeviceStats(1);
            UNIT_ASSERT_VALUES_EQUAL("dev2", dev1.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, dev1.GetErrors());
            UNIT_ASSERT_VALUES_EQUAL(2, dev1.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(2*DefaultBlockSize, dev1.GetBytesRead());
        });

        UNIT_ASSERT_EQUAL(E_IO, read("dev1"));
        UNIT_ASSERT_EQUAL(S_OK, read("dev2"));
        UNIT_ASSERT_EQUAL(S_OK, read("dev2"));
        UNIT_ASSERT_EQUAL(S_OK, read("dev2"));

        waitForStats([] (auto agentStats) {
            UNIT_ASSERT_EQUAL(2, agentStats.DeviceStatsSize());

            auto& dev0 = agentStats.GetDeviceStats(0);
            UNIT_ASSERT_VALUES_EQUAL("dev1", dev0.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, dev0.GetErrors());
            UNIT_ASSERT_VALUES_EQUAL(1, dev0.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(0, dev0.GetBytesRead());

            auto& dev1 = agentStats.GetDeviceStats(1);
            UNIT_ASSERT_VALUES_EQUAL("dev2", dev1.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, dev1.GetErrors());
            UNIT_ASSERT_VALUES_EQUAL(3, dev1.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(3*DefaultBlockSize, dev1.GetBytesRead());
        });
    }

    Y_UNIT_TEST(ShouldCollectStats)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({
                "MemoryDevice1",
                "MemoryDevice2",
                "MemoryDevice3",
            }))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        auto response = diskAgent.CollectStats();
        auto& stats = response->Stats;

        UNIT_ASSERT_VALUES_EQUAL(0, response->Stats.GetInitErrorsCount());
        UNIT_ASSERT_VALUES_EQUAL(3, stats.DeviceStatsSize());

        SortBy(*stats.MutableDeviceStats(), [] (auto& x) {
            return x.GetDeviceUUID();
        });

        UNIT_ASSERT_VALUES_EQUAL("MemoryDevice1", stats.GetDeviceStats(0).GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL("MemoryDevice2", stats.GetDeviceStats(1).GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL("MemoryDevice3", stats.GetDeviceStats(2).GetDeviceUUID());
    }

    Y_UNIT_TEST(ShouldPreserveCookie)
    {
        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({
                "MemoryDevice1",
                "MemoryDevice2",
                "MemoryDevice3",
            }))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids { "MemoryDevice1" };

        diskAgent.AcquireDevices(
            uuids,
            "session-id",
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        {
            const ui32 blocksCount = 10;

            TVector<TString> blocks;
            auto sglist = ResizeBlocks(
                blocks,
                blocksCount,
                TString(DefaultBlockSize, 'X'));

            auto request = diskAgent.CreateWriteDeviceBlocksRequest(
                uuids[0], 0, sglist, "session-id");

            const ui64 cookie = 42;

            diskAgent.SendRequest(MakeDiskAgentServiceId(), std::move(request), cookie);

            TAutoPtr<IEventHandle> handle;
            runtime.GrabEdgeEventRethrow<TEvDiskAgent::TEvWriteDeviceBlocksResponse>(
                handle, WaitTimeout);

            UNIT_ASSERT(handle);
            UNIT_ASSERT_VALUES_EQUAL(cookie, handle->Cookie);
        }

        {
            auto request = diskAgent.CreateSecureEraseDeviceRequest(
                "MemoryDevice2");

            const ui64 cookie = 42;

            diskAgent.SendRequest(MakeDiskAgentServiceId(), std::move(request), cookie);

            TAutoPtr<IEventHandle> handle;
            runtime.GrabEdgeEventRethrow<TEvDiskAgent::TEvSecureEraseDeviceResponse>(
                handle, WaitTimeout);

            UNIT_ASSERT(handle);
            UNIT_ASSERT_VALUES_EQUAL(cookie, handle->Cookie);
        }

        {
            auto request = diskAgent.CreateAcquireDevicesRequest(
                {"MemoryDevice2"},
                "session-d",
                NProto::VOLUME_ACCESS_READ_WRITE
            );

            const ui64 cookie = 42;

            diskAgent.SendRequest(MakeDiskAgentServiceId(), std::move(request), cookie);

            TAutoPtr<IEventHandle> handle;
            runtime.GrabEdgeEventRethrow<TEvDiskAgent::TEvAcquireDevicesResponse>(
                handle, WaitTimeout);

            UNIT_ASSERT(handle);
            UNIT_ASSERT_VALUES_EQUAL(cookie, handle->Cookie);
        }
    }

    Y_UNIT_TEST(ShouldPerformIoWithoutSpdk)
    {
        NProto::TDiskAgentConfig agentConfig;
        agentConfig.SetBackend(NProto::DISK_AGENT_BACKEND_AIO);
        agentConfig.SetAcquireRequired(true);
        agentConfig.SetEnabled(true);

        auto const workingDir = TryGetRamDrivePath();

        *agentConfig.AddFileDevices() = PrepareFileDevice(
            workingDir / "test",
            "FileDevice-1");

        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(agentConfig)
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        TVector<TString> uuids { "FileDevice-1" };

        const TString sessionId = "session-1";

        diskAgent.AcquireDevices(
            uuids,
            sessionId,
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        {
            auto response = diskAgent.CollectStats();

            UNIT_ASSERT_VALUES_EQUAL(0, response->Stats.GetInitErrorsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, response->Stats.GetDeviceStats().size());

            const auto& stats = response->Stats.GetDeviceStats(0);

            UNIT_ASSERT_VALUES_EQUAL("FileDevice-1", stats.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetBytesRead());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetBytesWritten());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetNumWriteOps());
        }

        const size_t startIndex = 512;
        const size_t blocksCount = 2;

        {
            TVector<TString> blocks;
            auto sglist = ResizeBlocks(
                blocks,
                blocksCount,
                TString(DefaultBlockSize, 'A'));

            diskAgent.SendWriteDeviceBlocksRequest(
                uuids[0],
                startIndex,
                sglist,
                sessionId);

            runtime.DispatchEvents(TDispatchOptions());

            auto response = diskAgent.RecvWriteDeviceBlocksResponse();

            UNIT_ASSERT(!HasError(response->Record));
        }

        {
            auto response = diskAgent.CollectStats();

            UNIT_ASSERT_VALUES_EQUAL(0, response->Stats.GetInitErrorsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, response->Stats.GetDeviceStats().size());

            const auto& stats = response->Stats.GetDeviceStats(0);

            UNIT_ASSERT_VALUES_EQUAL("FileDevice-1", stats.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetBytesRead());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize*blocksCount, stats.GetBytesWritten());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetNumWriteOps());
        }

        {
            const auto response = ReadDeviceBlocks(
                runtime, diskAgent, uuids[0], startIndex, blocksCount, sessionId);

            const auto& record = response->Record;

            UNIT_ASSERT(!HasError(record));

            auto sglist = ConvertToSgList(record.GetBlocks(), DefaultBlockSize);

            UNIT_ASSERT_VALUES_EQUAL(sglist.size(), blocksCount);

            for (const auto& buffer: sglist) {
                const char* ptr = buffer.Data();
                for (size_t i = 0; i < buffer.Size(); ++i) {
                    UNIT_ASSERT(ptr[i] == 'A');
                }
            }
        }

        {
            auto response = diskAgent.CollectStats();

            UNIT_ASSERT_VALUES_EQUAL(0, response->Stats.GetInitErrorsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, response->Stats.GetDeviceStats().size());

            const auto& stats = response->Stats.GetDeviceStats(0);

            UNIT_ASSERT_VALUES_EQUAL("FileDevice-1", stats.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize*blocksCount, stats.GetBytesRead());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize*blocksCount, stats.GetBytesWritten());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetNumWriteOps());
        }

        ZeroDeviceBlocks(runtime, diskAgent, uuids[0], startIndex, blocksCount, sessionId);

        {
            const auto response = ReadDeviceBlocks(
                runtime, diskAgent, uuids[0], startIndex, blocksCount, sessionId);

            const auto& record = response->Record;

            UNIT_ASSERT(!HasError(record));

            auto sglist = ConvertToSgList(record.GetBlocks(), DefaultBlockSize);

            UNIT_ASSERT_VALUES_EQUAL(sglist.size(), blocksCount);

            for (const auto& buffer: sglist) {
                const char* ptr = buffer.Data();
                for (size_t i = 0; i < buffer.Size(); ++i) {
                    UNIT_ASSERT(ptr[i] == 0);
                }
            }
        }

        {
            auto response = diskAgent.CollectStats();

            UNIT_ASSERT_VALUES_EQUAL(0, response->Stats.GetInitErrorsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, response->Stats.GetDeviceStats().size());

            const auto& stats = response->Stats.GetDeviceStats(0);

            UNIT_ASSERT_VALUES_EQUAL("FileDevice-1", stats.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(2 * DefaultBlockSize * blocksCount, stats.GetBytesRead());
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetNumReadOps());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize * blocksCount, stats.GetBytesWritten());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetNumWriteOps());
        }
    }

    Y_UNIT_TEST(ShouldHandleDeviceInitError)
    {
        NProto::TDiskAgentConfig agentConfig;
        agentConfig.SetBackend(NProto::DISK_AGENT_BACKEND_AIO);
        agentConfig.SetAgentId("agent-id");
        agentConfig.SetEnabled(true);

        auto const workingDir = TryGetRamDrivePath();

        *agentConfig.AddFileDevices() = PrepareFileDevice(
            workingDir / "test-1",
            "FileDevice-1");

        {
            NProto::TFileDeviceArgs& device = *agentConfig.AddFileDevices();

            device.SetPath(workingDir / "not-exists");
            device.SetBlockSize(DefaultDeviceBlockSize);
            device.SetDeviceId("FileDevice-2");
        }

        *agentConfig.AddFileDevices() = PrepareFileDevice(
            workingDir / "broken",
            "FileDevice-3");

        *agentConfig.AddFileDevices() = PrepareFileDevice(
            workingDir / "test-4",
            "FileDevice-4");

        TTestBasicRuntime runtime;

        NProto::TAgentConfig registeredAgent;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvDiskRegistry::EvRegisterAgentRequest: {
                        auto& msg = *event->Get<TEvDiskRegistry::TEvRegisterAgentRequest>();

                        registeredAgent = msg.Record.GetAgentConfig();
                    }
                    break;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        auto env = TTestEnvBuilder(runtime)
            .With(agentConfig)
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL("agent-id", registeredAgent.GetAgentId());
        UNIT_ASSERT_VALUES_EQUAL(4, registeredAgent.DevicesSize());

        auto& devices = *registeredAgent.MutableDevices();
        SortBy(devices.begin(), devices.end(), [] (const auto& device) {
            return device.GetDeviceUUID();
        });

        auto expectedBlocksCount =
            DefaultBlocksCount * DefaultBlockSize / DefaultDeviceBlockSize;

        UNIT_ASSERT_VALUES_EQUAL("FileDevice-1", devices[0].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL("the-rack", devices[0].GetRack());
        UNIT_ASSERT_VALUES_EQUAL(
            expectedBlocksCount,
            devices[0].GetBlocksCount()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<int>(NProto::DEVICE_STATE_ONLINE),
            static_cast<int>(devices[0].GetState())
        );

        UNIT_ASSERT_VALUES_EQUAL("FileDevice-2", devices[1].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL("the-rack", devices[1].GetRack());
        UNIT_ASSERT_VALUES_EQUAL(1, devices[1].GetBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<int>(NProto::DEVICE_STATE_ERROR),
            static_cast<int>(devices[1].GetState())
        );

        UNIT_ASSERT_VALUES_EQUAL("FileDevice-3", devices[2].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL("the-rack", devices[2].GetRack());
        UNIT_ASSERT_VALUES_EQUAL(
            expectedBlocksCount,
            devices[2].GetBlocksCount()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<int>(NProto::DEVICE_STATE_ERROR),
            static_cast<int>(devices[2].GetState())
        );

        UNIT_ASSERT_VALUES_EQUAL("FileDevice-4", devices[3].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL("the-rack", devices[3].GetRack());
        UNIT_ASSERT_VALUES_EQUAL(
            expectedBlocksCount,
            devices[3].GetBlocksCount()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<int>(NProto::DEVICE_STATE_ONLINE),
            static_cast<int>(devices[3].GetState())
        );

        auto response = diskAgent.CollectStats();
        const auto& stats = response->Stats;
        UNIT_ASSERT_VALUES_EQUAL(2, stats.GetInitErrorsCount());
    }

    Y_UNIT_TEST(ShouldHandleSpdkInitError)
    {
        TVector<NProto::TDeviceConfig> devices;

        NProto::TDiskAgentConfig agentConfig;
        agentConfig.SetEnabled(true);
        agentConfig.SetAgentId("agent");
        agentConfig.SetBackend(NProto::DISK_AGENT_BACKEND_SPDK);

        for (size_t i = 0; i < 10; i++) {
            auto& device = devices.emplace_back();
            device.SetDeviceName(Sprintf("%s%zu", (i & 1 ? "broken" : "file"), i));
            device.SetDeviceUUID(CreateGuidAsString());

            auto* config = agentConfig.MutableFileDevices()->Add();
            config->SetPath(device.GetDeviceName());
            config->SetDeviceId(device.GetDeviceUUID());
            config->SetBlockSize(4096);
        }

        auto nvmeConfig = agentConfig.MutableNvmeDevices()->Add();
        nvmeConfig->SetBaseName("broken");

        for (size_t i = 0; i < 10; i++) {
            auto& device = devices.emplace_back();
            device.SetDeviceName(Sprintf("agent:broken:n%zu", i));
            device.SetDeviceUUID(CreateGuidAsString());

            *nvmeConfig->MutableDeviceIds()->Add() = device.GetDeviceUUID();
        }

        TTestBasicRuntime runtime;
        TTestEnv env = TTestEnvBuilder(runtime)
            .With(agentConfig)
            .With(std::make_shared<TTestSpdkEnv>(devices))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL(devices.size(), env.DiskRegistryState->Devices.size());

        for (const auto& expected: devices) {
            const auto& deviceName = expected.GetDeviceName();
            const auto& device = env.DiskRegistryState->Devices.at(deviceName);

            UNIT_ASSERT_VALUES_EQUAL(device.GetDeviceUUID(), expected.GetDeviceUUID());

            if (deviceName.Contains("broken")) {
                UNIT_ASSERT_EQUAL(device.GetState(), NProto::DEVICE_STATE_ERROR);
            } else {
                UNIT_ASSERT_EQUAL(device.GetState(), NProto::DEVICE_STATE_ONLINE);
            }
        }
    }

    Y_UNIT_TEST(ShouldSecureEraseAioDevice)
    {
        NProto::TDiskAgentConfig agentConfig;
        agentConfig.SetBackend(NProto::DISK_AGENT_BACKEND_AIO);
        agentConfig.SetDeviceEraseMethod(NProto::DEVICE_ERASE_METHOD_CRYPTO_ERASE);
        agentConfig.SetAgentId("agent-id");
        agentConfig.SetEnabled(true);

        auto const workingDir = TryGetRamDrivePath();

        *agentConfig.AddFileDevices() = PrepareFileDevice(
            workingDir / "test-1",
            "FileDevice-1");

        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(agentConfig)
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();
        diskAgent.SecureEraseDevice("FileDevice-1");
    }

    Y_UNIT_TEST(ShouldZeroFillDevice)
    {
        NProto::TDiskAgentConfig agentConfig;
        agentConfig.SetBackend(NProto::DISK_AGENT_BACKEND_AIO);
        agentConfig.SetDeviceEraseMethod(NProto::DEVICE_ERASE_METHOD_ZERO_FILL);
        agentConfig.SetAgentId("agent-id");
        agentConfig.SetEnabled(true);
        agentConfig.SetAcquireRequired(true);

        auto const workingDir = TryGetRamDrivePath();

        *agentConfig.AddFileDevices() = PrepareFileDevice(
            workingDir / "test-1",
            "FileDevice-1");

        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(agentConfig)
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        const TString sessionId = "session-1";
        const TVector<TString> uuids { "FileDevice-1" };

        diskAgent.AcquireDevices(
            uuids,
            sessionId,
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        const ui64 startIndex = 10;
        const ui32 blockCount = 16_KB / DefaultBlockSize;
        const TString content(DefaultBlockSize, 'A');

        TVector<TString> blocks;
        auto sglist = ResizeBlocks(
            blocks,
            blockCount,
            content);

        WriteDeviceBlocks(
            runtime, diskAgent, uuids[0], startIndex, sglist, sessionId);

        {
            auto response = ReadDeviceBlocks(
                runtime,
                diskAgent,
                uuids[0],
                startIndex,
                blockCount,
                sessionId);

            auto data = ConvertToSgList(
                response->Record.GetBlocks(),
                DefaultBlockSize);

            UNIT_ASSERT_VALUES_EQUAL(blockCount, data.size());
            for (TBlockDataRef dr: data) {
                UNIT_ASSERT_STRINGS_EQUAL(content, dr.AsStringBuf());
            }
        }

        diskAgent.ReleaseDevices(uuids, sessionId);

        diskAgent.SecureEraseDevice("FileDevice-1");

        diskAgent.AcquireDevices(
            uuids,
            sessionId,
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        {
            auto response = ReadDeviceBlocks(
                runtime,
                diskAgent,
                uuids[0],
                startIndex,
                blockCount,
                sessionId);

            auto data = ConvertToSgList(
                response->Record.GetBlocks(),
                DefaultBlockSize);

            UNIT_ASSERT_VALUES_EQUAL(blockCount, data.size());

            for (TBlockDataRef dr: data) {
                for (char c: dr.AsStringBuf()) {
                    UNIT_ASSERT_VALUES_EQUAL(0, c);
                }
            }
        }

        diskAgent.ReleaseDevices(uuids, sessionId);
    }

    Y_UNIT_TEST(ShouldInjectExceptions)
    {
        NLWTrace::TManager traceManager(
            *Singleton<NLWTrace::TProbeRegistry>(),
            true);  // allowDestructiveActions

        traceManager.RegisterCustomAction(
            "ServiceErrorAction", &CreateServiceErrorActionExecutor);

        // read errors for MemoryDevice1 & zero errors for MemoryDevice2
        traceManager.New("env", QueryFromString(R"(
            Blocks {
                ProbeDesc {
                    Name: "FaultInjection"
                    Provider: "BLOCKSTORE_DISK_AGENT_PROVIDER"
                }
                Action {
                    StatementAction {
                        Type: ST_INC
                        Argument { Variable: "counter1" }
                    }
                }
                Action {
                    StatementAction {
                        Type: ST_MOD
                        Argument { Variable: "counter1" }
                        Argument { Variable: "counter1" }
                        Argument { Value: "100" }
                    }
                }
                Predicate {
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "name" }
                        Argument { Value: "ReadDeviceBlocks" }
                    }
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "deviceId" }
                        Argument { Value: "MemoryDevice1" }
                    }
                }
            }
            Blocks {
                ProbeDesc {
                    Name: "FaultInjection"
                    Provider: "BLOCKSTORE_DISK_AGENT_PROVIDER"
                }
                Action {
                    CustomAction {
                        Name: "ServiceErrorAction"
                        Opts: "E_IO"
                        Opts: "Io Error"
                    }
                }
                Predicate {
                    Operators {
                        Type: OT_EQ
                        Argument { Variable: "counter1" }
                        Argument { Value: "99" }
                    }
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "name" }
                        Argument { Value: "ReadDeviceBlocks" }
                    }
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "deviceId" }
                        Argument { Value: "MemoryDevice1" }
                    }
                }
            }

            Blocks {
                ProbeDesc {
                    Name: "FaultInjection"
                    Provider: "BLOCKSTORE_DISK_AGENT_PROVIDER"
                }
                Action {
                    StatementAction {
                        Type: ST_INC
                        Argument { Variable: "counter2" }
                    }
                }
                Action {
                    StatementAction {
                        Type: ST_MOD
                        Argument { Variable: "counter2" }
                        Argument { Variable: "counter2" }
                        Argument { Value: "100" }
                    }
                }
                Predicate {
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "name" }
                        Argument { Value: "ZeroDeviceBlocks" }
                    }
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "deviceId" }
                        Argument { Value: "MemoryDevice2" }
                    }
                }
            }
            Blocks {
                ProbeDesc {
                    Name: "FaultInjection"
                    Provider: "BLOCKSTORE_DISK_AGENT_PROVIDER"
                }
                Action {
                    CustomAction {
                        Name: "ServiceErrorAction"
                        Opts: "E_IO"
                        Opts: "Io Error"
                    }
                }
                Predicate {
                    Operators {
                        Type: OT_EQ
                        Argument { Variable: "counter2" }
                        Argument { Value: "99" }
                    }
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "name" }
                        Argument { Value: "ZeroDeviceBlocks" }
                    }
                    Operators {
                        Type: OT_EQ
                        Argument { Param: "deviceId" }
                        Argument { Value: "MemoryDevice2" }
                    }
                }
            }
        )"));

        TTestBasicRuntime runtime;

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({
                "MemoryDevice1",
                "MemoryDevice2",
                "MemoryDevice3",
            }))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids{
            "MemoryDevice1",
            "MemoryDevice2"
        };

        const TString sessionId = "session-1";

        diskAgent.AcquireDevices(
            uuids,
            sessionId,
            NProto::VOLUME_ACCESS_READ_WRITE);

        auto read = [&] (int index) {
            const auto response = ReadDeviceBlocks(
                runtime, diskAgent, uuids[index], 0, 1, sessionId);

            return response->GetStatus() == E_IO;
        };

        TString data(DefaultBlockSize, 'X');
        TSgList sglist = {{ data.data(), data.size() }};

        auto write = [&] (int index) {
            diskAgent.SendWriteDeviceBlocksRequest(uuids[index], 0, sglist, sessionId);
            runtime.DispatchEvents(TDispatchOptions());
            auto response = diskAgent.RecvWriteDeviceBlocksResponse();

            return response->GetStatus() == E_IO;
        };

        auto zero = [&] (int index) {
            diskAgent.SendZeroDeviceBlocksRequest(uuids[index], 0, 1, sessionId);
            runtime.DispatchEvents(TDispatchOptions());
            const auto response = diskAgent.RecvZeroDeviceBlocksResponse();

            return response->GetStatus() == E_IO;
        };

        {
            int errors = 0;

            for (int i = 0; i != 1000; ++i) {
                errors += read(0);
            }

            UNIT_ASSERT_VALUES_EQUAL(1000 / 100, errors);
        }

        for (int i = 0; i != 2000; ++i) {
            UNIT_ASSERT(!read(1));

            UNIT_ASSERT(!write(0));
            UNIT_ASSERT(!write(1));

            UNIT_ASSERT(!zero(0));
        }

        {
            int errors = 0;

            for (int i = 0; i != 1000; ++i) {
                errors += zero(1);
            }

            UNIT_ASSERT_VALUES_EQUAL(1000 / 100, errors);
        }
    }

    Y_UNIT_TEST(ShouldRegisterAfterDisconnect)
    {
        int registrationCount = 0;

        TTestBasicRuntime runtime;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvDiskRegistry::EvRegisterAgentRequest:
                        ++registrationCount;
                    break;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig({"MemoryDevice1"}))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(1, registrationCount);

        diskAgent.SendRequest(
            MakeDiskAgentServiceId(),
            std::make_unique<TEvDiskRegistryProxy::TEvConnectionLost>());

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(2, registrationCount);
    }

    Y_UNIT_TEST(ShouldSecureEraseDeviceWithExpiredSession)
    {
        TTestBasicRuntime runtime;

        const TVector<TString> uuids {
            "MemoryDevice1",
            "MemoryDevice2",
            "MemoryDevice3"
        };

        auto env = TTestEnvBuilder(runtime)
            .With(DiskAgentConfig(uuids))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        diskAgent.AcquireDevices(
            uuids,
            "session-1",
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        auto secureErase = [&] (auto expectedErrorCode) {
            for (const auto& uuid: uuids) {
                diskAgent.SendSecureEraseDeviceRequest(uuid);

                auto response = diskAgent.RecvSecureEraseDeviceResponse();
                UNIT_ASSERT_VALUES_EQUAL(
                    expectedErrorCode,
                    response->Record.GetError().GetCode()
                );
            }
        };

        secureErase(E_INVALID_STATE);

        runtime.AdvanceCurrentTime(TDuration::Seconds(10));

        secureErase(S_OK);
    }

    Y_UNIT_TEST(ShouldRegisterDevices)
    {
        TVector<NProto::TDeviceConfig> devices;
        devices.reserve(15);
        for (size_t i = 0; i != 15; ++i) {
            auto& device = devices.emplace_back();

            device.SetDeviceName(Sprintf("/dev/disk/by-partlabel/NVMENBS%02lu", i + 1));
            device.SetBlockSize(4096);
            device.SetDeviceUUID(CreateGuidAsString());
            if (i != 11) {
                device.SetBlocksCount(24151552);
            } else {
                device.SetBlocksCount(24169728);
            }
        }

        TTestBasicRuntime runtime;

        TTestEnv env = TTestEnvBuilder(runtime)
            .With([&] {
                NProto::TDiskAgentConfig agentConfig;
                agentConfig.SetEnabled(true);
                for (const auto& device: devices) {
                    auto* config = agentConfig.MutableFileDevices()->Add();
                    config->SetPath(device.GetDeviceName());
                    config->SetBlockSize(device.GetBlockSize());
                    config->SetDeviceId(device.GetDeviceUUID());
                }

                return agentConfig;
            }())
            .With(std::make_shared<TTestSpdkEnv>(devices))
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL(
            devices.size(),
            env.DiskRegistryState->Devices.size()
        );

        for (const auto& expected: devices) {
            const auto& device = env.DiskRegistryState->Devices.at(
                expected.GetDeviceName()
            );

            UNIT_ASSERT_VALUES_EQUAL(
                expected.GetDeviceUUID(),
                device.GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                expected.GetDeviceName(),
                device.GetDeviceName()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                expected.GetBlockSize(),
                device.GetBlockSize()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                expected.GetBlocksCount(),
                device.GetBlocksCount()
            );
        }
    }

    Y_UNIT_TEST(ShouldLimitSecureEraseRequests)
    {
        TTestBasicRuntime runtime;

        auto config = DiskAgentConfig({"foo", "bar"});
        config.SetDeviceEraseMethod(NProto::DEVICE_ERASE_METHOD_USER_DATA_ERASE);

        TVector<NProto::TDeviceConfig> devices;

        for (auto& md: config.GetMemoryDevices()) {
            auto& device = devices.emplace_back();
            device.SetDeviceName(md.GetName());
            device.SetDeviceUUID(md.GetDeviceId());
            device.SetBlocksCount(md.GetBlocksCount());
            device.SetBlockSize(md.GetBlockSize());
        }

        auto spdk = std::make_shared<TTestSpdkEnv>(devices);

        auto env = TTestEnvBuilder(runtime)
            .With(config)
            .With(spdk)
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        auto foo = NewPromise<NProto::TError>();
        auto bar = NewPromise<NProto::TError>();

        spdk->SecureEraseResult = foo;

        for (int i = 0; i != 100; ++i) {
            diskAgent.SendSecureEraseDeviceRequest("foo");
        }

        runtime.DispatchEvents(TDispatchOptions(), TDuration::MilliSeconds(10));
        UNIT_ASSERT_VALUES_EQUAL(1, AtomicGet(spdk->SecureEraseCount));

        for (int i = 0; i != 100; ++i) {
            diskAgent.SendSecureEraseDeviceRequest("bar");
        }

        runtime.DispatchEvents(TDispatchOptions(), TDuration::MilliSeconds(10));
        UNIT_ASSERT_VALUES_EQUAL(1, AtomicGet(spdk->SecureEraseCount));

        spdk->SecureEraseResult = bar;
        foo.SetValue(NProto::TError());

        for (int i = 0; i != 100; ++i) {
            auto response = diskAgent.RecvSecureEraseDeviceResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetError().GetCode()
            );
        }

        bar.SetValue(NProto::TError());

        for (int i = 0; i != 100; ++i) {
            auto response = diskAgent.RecvSecureEraseDeviceResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetError().GetCode()
            );
        }

        UNIT_ASSERT_VALUES_EQUAL(2, AtomicGet(spdk->SecureEraseCount));
    }

    Y_UNIT_TEST(ShouldRejectSecureEraseRequestsOnPoisonPill)
    {
        TTestBasicRuntime runtime;

        NProto::TDeviceConfig device;

        device.SetDeviceName("uuid");
        device.SetDeviceUUID("uuid");
        device.SetBlocksCount(1024);
        device.SetBlockSize(DefaultBlockSize);

        auto spdk = std::make_shared<TTestSpdkEnv>(TVector{device});

        auto env = TTestEnvBuilder(runtime)
            .With([]{
                auto config = DiskAgentConfig({"uuid"});
                config.SetDeviceEraseMethod(NProto::DEVICE_ERASE_METHOD_USER_DATA_ERASE);
                return config;
            }())
            .With(spdk)
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        spdk->SecureEraseResult = NewPromise<NProto::TError>();

        for (int i = 0; i != 100; ++i) {
            diskAgent.SendSecureEraseDeviceRequest("uuid");
        }

        diskAgent.SendRequest(
            MakeDiskAgentServiceId(),
            std::make_unique<TEvents::TEvPoisonPill>());

        for (int i = 0; i != 100; ++i) {
            auto response = diskAgent.RecvSecureEraseDeviceResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                E_REJECTED,
                response->Record.GetError().GetCode()
            );
        }

        UNIT_ASSERT_VALUES_EQUAL(1, AtomicGet(spdk->SecureEraseCount));
    }

    Y_UNIT_TEST(ShouldRejectIORequestsDuringSecureErase)
    {
        TTestBasicRuntime runtime;

        NProto::TDeviceConfig device;

        device.SetDeviceName("uuid");
        device.SetDeviceUUID("uuid");
        device.SetBlocksCount(1024);
        device.SetBlockSize(DefaultBlockSize);

        auto spdk = std::make_shared<TTestSpdkEnv>(TVector{device});

        auto env = TTestEnvBuilder(runtime)
            .With([]{
                auto config = DiskAgentConfig({"uuid"}, false);
                config.SetDeviceEraseMethod(NProto::DEVICE_ERASE_METHOD_USER_DATA_ERASE);
                return config;
            }())
            .With(spdk)
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        spdk->SecureEraseResult = NewPromise<NProto::TError>();

        diskAgent.SendSecureEraseDeviceRequest("uuid");

        auto io = [&] (auto expectedErrorCode) {
            UNIT_ASSERT_VALUES_EQUAL(
                expectedErrorCode,
                ReadDeviceBlocks(runtime, diskAgent, "uuid", 0, 1, "")
                    ->Record.GetError().GetCode());

            UNIT_ASSERT_VALUES_EQUAL(
                expectedErrorCode,
                ZeroDeviceBlocks(runtime, diskAgent, "uuid", 0, 1, "")
                    ->Record.GetError().GetCode());

            {
                TVector<TString> blocks;
                auto sglist = ResizeBlocks(
                    blocks,
                    1,
                    TString(DefaultBlockSize, 'X'));

                UNIT_ASSERT_VALUES_EQUAL(
                    expectedErrorCode,
                    WriteDeviceBlocks(runtime, diskAgent, "uuid", 0, sglist, "")
                        ->Record.GetError().GetCode());
            }
        };

        io(E_REJECTED);

        spdk->SecureEraseResult.SetValue(NProto::TError());
        runtime.DispatchEvents(TDispatchOptions(), TDuration::MilliSeconds(10));

        io(S_OK);

        {
            auto response = diskAgent.RecvSecureEraseDeviceResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetError().GetCode()
            );
        }

        UNIT_ASSERT_VALUES_EQUAL(1, AtomicGet(spdk->SecureEraseCount));
    }

    Y_UNIT_TEST(ShouldChecksumDeviceBlocks)
    {
        TTestBasicRuntime runtime;

        const auto blockSize = DefaultBlockSize;
        const auto blocksCount = 10;

        auto env = TTestEnvBuilder(runtime)
            .With([&] {
                NProto::TDiskAgentConfig config;
                config.SetBackend(NProto::DISK_AGENT_BACKEND_AIO);
                config.SetAcquireRequired(true);
                config.SetEnabled(true);

                *config.AddMemoryDevices() = PrepareMemoryDevice(
                    "MemoryDevice1",
                    blockSize,
                    blocksCount*blockSize);

                return config;
            }())
            .Build();

        TDiskAgentClient diskAgent(runtime);
        diskAgent.WaitReady();

        runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        const TVector<TString> uuids{
            "MemoryDevice1",
        };

        const TString sessionId = "session-1";

        diskAgent.AcquireDevices(
            uuids,
            sessionId,
            NProto::VOLUME_ACCESS_READ_WRITE
        );

        TBlockChecksum checksum;

        {
            TVector<TString> blocks;
            auto sglist = ResizeBlocks(
                blocks,
                blocksCount,
                TString(blockSize, 'X'));

            for (const auto& block : blocks) {
                checksum.Extend(block.Data(), block.Size());
            }

            WriteDeviceBlocks(runtime, diskAgent, uuids[0], 0, sglist, sessionId);
        }

        {
            const auto response = ChecksumDeviceBlocks(
                runtime, diskAgent, uuids[0], 0, blocksCount, sessionId);

            const auto& record = response->Record;

            UNIT_ASSERT_VALUES_EQUAL(checksum.GetValue(), record.GetChecksum());
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
