#include "device_list.h"

#include "agent_list.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/iterator_range.h>
#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr ui64 DefaultBlockSize = 4_KB;
constexpr ui64 DefaultBlockCount = 93_GB / DefaultBlockSize;
const TString DefaultPoolName;

////////////////////////////////////////////////////////////////////////////////

auto CreateAgentConfig(const TString& id, ui32 nodeId, const TString& rack)
{
    NProto::TAgentConfig agent;
    agent.SetAgentId(id);
    agent.SetNodeId(nodeId);

    for (int i = 0; i != 15; ++i) {
        auto* device = agent.AddDevices();
        device->SetBlockSize(DefaultBlockSize);
        device->SetBlocksCount(DefaultBlockCount);
        device->SetUnadjustedBlockCount(DefaultBlockCount);
        device->SetRack(rack);
        device->SetNodeId(agent.GetNodeId());
        device->SetDeviceUUID(id + "-" + ToString(i + 1));
    }

    return agent;
}

template <typename C>
ui64 CalcTotalSize(const C& devices)
{
    return Accumulate(devices, ui64{}, [] (ui64 val, auto& x) {
        return val + x.GetBlockSize() * x.GetBlocksCount();
    });
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDeviceListTest)
{
    Y_UNIT_TEST(ShouldAllocateSingleDevice)
    {
        TDeviceList deviceList({}, {});

        auto allocate = [&] (THashSet<TString> racks) {
            return deviceList.AllocateDevice(
                "disk",
                {
                    .OtherRacks = std::move(racks),
                    .LogicalBlockSize = DefaultBlockSize,
                    .BlockCount = DefaultBlockCount
                }
            ).GetDeviceUUID();
        };

        UNIT_ASSERT(allocate({}).empty());

        NProto::TAgentConfig foo = CreateAgentConfig("foo", 1000, "rack-1");
        NProto::TAgentConfig bar = CreateAgentConfig("bar", 2000, "rack-2");
        NProto::TAgentConfig baz = CreateAgentConfig("baz", 3000, "rack-3");

        foo.MutableDevices(6)->SetState(NProto::DEVICE_STATE_WARNING);
        bar.MutableDevices(6)->SetState(NProto::DEVICE_STATE_WARNING);
        baz.MutableDevices(6)->SetState(NProto::DEVICE_STATE_WARNING);
        baz.MutableDevices(9)->SetState(NProto::DEVICE_STATE_ERROR);

        deviceList.UpdateDevices(foo);
        deviceList.UpdateDevices(bar);
        deviceList.UpdateDevices(baz);

        deviceList.MarkDeviceAsDirty("foo-13");
        deviceList.MarkDeviceAsDirty("bar-13");
        deviceList.MarkDeviceAsDirty("baz-13");

        UNIT_ASSERT_VALUES_EQUAL(3, deviceList.GetDirtyDevices().size());

        UNIT_ASSERT(!allocate({}).empty());
        UNIT_ASSERT(allocate({"rack-1", "rack-2", "rack-3"}).empty());
        UNIT_ASSERT(allocate({"rack-2", "rack-3"}).StartsWith("foo-"));
        UNIT_ASSERT(allocate({"rack-1", "rack-3"}).StartsWith("bar-"));
        UNIT_ASSERT(allocate({"rack-1", "rack-2"}).StartsWith("baz-"));

        // total = 15 * 3; dirty = 3; allocated = 4; warn = 3; error = 1
        for (int i = 15 * 3 - 3 - 4 - 3 - 1; i; --i) {
            UNIT_ASSERT(!allocate({}).empty());
        }

        UNIT_ASSERT(allocate({}).empty());
        UNIT_ASSERT_VALUES_EQUAL(3, deviceList.GetDirtyDevices().size());

        UNIT_ASSERT(!deviceList.ReleaseDevice("unknown"));
        UNIT_ASSERT(deviceList.ReleaseDevice("foo-1"));
        UNIT_ASSERT_VALUES_EQUAL(4, deviceList.GetDirtyDevices().size());
        UNIT_ASSERT(allocate({}).empty());

        UNIT_ASSERT(deviceList.MarkDeviceAsClean("foo-1"));
        deviceList.UpdateDevices(foo);

        UNIT_ASSERT(allocate({"rack-1"}).empty());
        UNIT_ASSERT_VALUES_EQUAL("foo-1", allocate({}));
    }

    Y_UNIT_TEST(ShouldAllocateDevices)
    {
        TDeviceList deviceList({}, {});

        auto allocate = [&] (ui32 n, THashSet<TString> racks) {
            return deviceList.AllocateDevices(
                "disk",
                {
                    .OtherRacks = std::move(racks),
                    .LogicalBlockSize = DefaultBlockSize,
                    .BlockCount = n * DefaultBlockCount
                }
            );
        };

        UNIT_ASSERT_VALUES_EQUAL(0, deviceList.Size());
        UNIT_ASSERT(allocate(1, {}).empty());
        UNIT_ASSERT(allocate(8, {}).empty());

        NProto::TAgentConfig foo = CreateAgentConfig("foo", 1000, "rack-1");
        NProto::TAgentConfig bar = CreateAgentConfig("bar", 2000, "rack-2");
        NProto::TAgentConfig baz = CreateAgentConfig("baz", 3000, "rack-3");

        foo.MutableDevices(6)->SetState(NProto::DEVICE_STATE_WARNING);
        bar.MutableDevices(6)->SetState(NProto::DEVICE_STATE_WARNING);
        baz.MutableDevices(6)->SetState(NProto::DEVICE_STATE_WARNING);
        baz.MutableDevices(9)->SetState(NProto::DEVICE_STATE_ERROR);

        deviceList.UpdateDevices(foo);
        UNIT_ASSERT_VALUES_EQUAL(15, deviceList.Size());

        deviceList.UpdateDevices(bar);
        UNIT_ASSERT_VALUES_EQUAL(30, deviceList.Size());

        deviceList.UpdateDevices(baz);
        UNIT_ASSERT_VALUES_EQUAL(45, deviceList.Size());

        UNIT_ASSERT_VALUES_EQUAL(0, deviceList.GetDirtyDevices().size());

        UNIT_ASSERT(allocate(1, {"rack-1", "rack-2", "rack-3"}).empty());
        UNIT_ASSERT(allocate(15, {"rack-2", "rack-3"}).empty());
        UNIT_ASSERT(allocate(15, {"rack-1", "rack-3"}).empty());
        UNIT_ASSERT(allocate(15, {"rack-1", "rack-2"}).empty());

        deviceList.MarkDeviceAsDirty("foo-13");
        deviceList.MarkDeviceAsDirty("bar-13");
        deviceList.MarkDeviceAsDirty("baz-13");

        UNIT_ASSERT_VALUES_EQUAL(3, deviceList.GetDirtyDevices().size());

        {
            auto devices = allocate(1, {"rack-2", "rack-3"});
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT(devices[0].GetDeviceUUID().StartsWith("foo-"));
        }

        {
            auto devices = allocate(1, {"rack-1", "rack-3"});
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT(devices[0].GetDeviceUUID().StartsWith("bar-"));
        }

        {
            auto devices = allocate(1, {"rack-1", "rack-2"});
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT(devices[0].GetDeviceUUID().StartsWith("baz-"));
        }

        // total = 45; warn = 3; error = 1; allocated = 3

        {
            const ui64 n = 7;

            auto devices = allocate(n, {});
            UNIT_ASSERT_VALUES_EQUAL(n, devices.size());

            const ui64 size = CalcTotalSize(devices);
            UNIT_ASSERT_VALUES_EQUAL(
                n * DefaultBlockSize * DefaultBlockCount,
                size);
        }

        // total = 45; dirty = 3; warn = 3; error = 1; allocated = 10

        {
            const ui64 n = 10;
            auto devices = allocate(n, {"rack-1", "rack-3"});
            UNIT_ASSERT_VALUES_EQUAL(n, devices.size());

            const ui64 size = CalcTotalSize(devices);
            UNIT_ASSERT_VALUES_EQUAL(
                n * DefaultBlockSize * DefaultBlockCount,
                size);
        }

        // total = 45; dirty = 3; warn = 3; error = 1; allocated = 20

        {
            const ui64 n = 45 - 3 - 3 - 1 - 20;
            auto devices = allocate(n, {});
            UNIT_ASSERT_VALUES_EQUAL(n, devices.size());

            const ui64 size = CalcTotalSize(devices);
            UNIT_ASSERT_VALUES_EQUAL(
                n * DefaultBlockSize * DefaultBlockCount,
                size);
        }

        UNIT_ASSERT_VALUES_EQUAL(0, allocate(1, {}).size());

        THashSet<TString> discardedDevices {
            "foo-7", "bar-7", "baz-7", "baz-10", "foo-13", "bar-13", "baz-13"
        };

        for (auto* agent: {&foo, &bar, &baz}) {
            for (auto& device: agent->GetDevices()) {
                const auto& uuid = device.GetDeviceUUID();
                auto diskId = deviceList.FindDiskId(uuid);

                if (discardedDevices.contains(uuid)) {
                    UNIT_ASSERT(diskId.empty());
                } else {
                    UNIT_ASSERT_VALUES_EQUAL("disk", diskId);
                }
            }
        }

        UNIT_ASSERT(deviceList.MarkDeviceAsClean("foo-13"));
        deviceList.UpdateDevices(foo);
        UNIT_ASSERT(deviceList.MarkDeviceAsClean("bar-13"));
        deviceList.UpdateDevices(bar);
        UNIT_ASSERT(deviceList.MarkDeviceAsClean("baz-13"));
        deviceList.UpdateDevices(baz);

        UNIT_ASSERT_VALUES_EQUAL(0, allocate(3, {"rack-1"}).size());
        UNIT_ASSERT_VALUES_EQUAL(0, allocate(3, {"rack-2"}).size());
        UNIT_ASSERT_VALUES_EQUAL(0, allocate(3, {"rack-3"}).size());

        {
            const ui64 n = 3;
            auto devices = allocate(n, {});
            UNIT_ASSERT_VALUES_EQUAL(n, devices.size());

            const ui64 size = CalcTotalSize(devices);
            UNIT_ASSERT_VALUES_EQUAL(
                n * DefaultBlockSize * DefaultBlockCount,
                size);
        }
    }

    Y_UNIT_TEST(ShouldMinimizeRackCountUponDeviceAllocation)
    {
        TDeviceList deviceList({}, {});

        auto allocate = [&] (ui32 n, THashSet<TString> racks) {
            return deviceList.AllocateDevices(
                "disk",
                {
                    .OtherRacks = std::move(racks),
                    .LogicalBlockSize = DefaultBlockSize,
                    .BlockCount = n * DefaultBlockCount
                }
            );
        };

        UNIT_ASSERT_VALUES_EQUAL(0, deviceList.Size());
        UNIT_ASSERT(allocate(1, {}).empty());
        UNIT_ASSERT(allocate(8, {}).empty());

        // 10 agents per rack => 150 devices per rack
        for (ui32 rack = 0; rack < 5; ++rack) {
            for (ui32 agent = 0; agent < 10; ++agent) {
                NProto::TAgentConfig config = CreateAgentConfig(
                    Sprintf("r%ua%u", rack, agent),
                    rack * 100 + agent,
                    Sprintf("rack-%u", rack));

                deviceList.UpdateDevices(config);
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(5 * 10 * 15, deviceList.Size());

        UNIT_ASSERT_VALUES_EQUAL(0, deviceList.GetDirtyDevices().size());

        auto devices1 = allocate(100, {});
        UNIT_ASSERT_VALUES_EQUAL(100, devices1.size());
        const auto& rack1 = devices1[0].GetRack();

        for (const auto& device: devices1) {
            UNIT_ASSERT_VALUES_EQUAL_C(
                rack1,
                device.GetRack(),
                device.GetAgentId());
        }

        auto devices2 = allocate(100, {});
        UNIT_ASSERT_VALUES_EQUAL(100, devices2.size());
        const auto& rack2 = devices2[0].GetRack();

        UNIT_ASSERT_VALUES_UNEQUAL(rack1, rack2);

        for (const auto& device: devices2) {
            UNIT_ASSERT_VALUES_EQUAL_C(
                rack2,
                device.GetRack(),
                device.GetAgentId());
        }
    }

    Y_UNIT_TEST(ShouldSelectRackWithTheMostSpaceUponDeviceAllocation)
    {
        TDeviceList deviceList({}, {});

        auto allocate = [&] (ui32 n, THashSet<TString> racks) {
            return deviceList.AllocateDevices(
                "disk",
                {
                    .OtherRacks = std::move(racks),
                    .LogicalBlockSize = DefaultBlockSize,
                    .BlockCount = n * DefaultBlockCount
                }
            );
        };

        NProto::TAgentConfig agent1 = CreateAgentConfig("agent1", 1, "rack1");
        NProto::TAgentConfig agent2 = CreateAgentConfig("agent2", 2, "rack2");
        NProto::TAgentConfig agent3 = CreateAgentConfig("agent3", 3, "rack2");
        deviceList.UpdateDevices(agent1);
        deviceList.UpdateDevices(agent2);
        deviceList.UpdateDevices(agent3);

        UNIT_ASSERT_VALUES_EQUAL(45, deviceList.Size());

        auto devices1 = allocate(10, {});
        UNIT_ASSERT_VALUES_EQUAL(10, devices1.size());
        UNIT_ASSERT_VALUES_EQUAL("rack2", devices1[0].GetRack());

        auto devices2 = allocate(10, {});
        UNIT_ASSERT_VALUES_EQUAL(10, devices2.size());
        UNIT_ASSERT_VALUES_EQUAL("rack2", devices2[0].GetRack());

        auto devices3 = allocate(1, {});
        UNIT_ASSERT_VALUES_EQUAL(1, devices3.size());
        UNIT_ASSERT_VALUES_EQUAL("rack1", devices3[0].GetRack());
    }

    Y_UNIT_TEST(ShouldAllocateFromSpecifiedNode)
    {
        TDeviceList deviceList({}, {});

        TVector agents {
            CreateAgentConfig("agent1", 1, "rack1"),
            CreateAgentConfig("agent2", 2, "rack2"),
            CreateAgentConfig("agent3", 3, "rack2")
        };

        for (const auto& agent: agents) {
            UNIT_ASSERT_VALUES_EQUAL(15, agent.DevicesSize());
            deviceList.UpdateDevices(agent);
        }

        auto allocate = [&] (ui32 n, ui32 nodeId) {
            return deviceList.AllocateDevices(
                Sprintf("disk-%d-%d", n, nodeId),
                {
                    .OtherRacks = {},
                    .LogicalBlockSize = DefaultBlockSize,
                    .BlockCount = n * DefaultBlockCount,
                    .NodeIds = { nodeId }
                }
            );
        };

        // allocate one device from each agent
        for (const auto& agent: agents) {
            const auto nodeId = agent.GetNodeId();
            auto devices = allocate(1, nodeId);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(nodeId, devices[0].GetNodeId());
        }

        for (const auto& agent: agents) {
            auto devices = allocate(15, agent.GetNodeId());
            // can't allocate 15 devices
            UNIT_ASSERT_VALUES_EQUAL(0, devices.size());
        }

        // allocate 14 devices from each agent
        for (const auto& agent: agents) {
            const auto nodeId = agent.GetNodeId();

            auto devices = allocate(14, nodeId);
            UNIT_ASSERT_VALUES_EQUAL(14, devices.size());
            for (auto& d: devices) {
                UNIT_ASSERT_VALUES_EQUAL(nodeId, d.GetNodeId());
            }
        }
    }

    // TODO: allocate by pool name
    /*
    Y_UNIT_TEST(ShouldAllocateFromSpecifiedNodeByTag)
    {
        const TString ssdLocalTag = "ssd_local";

        TDeviceList deviceList({}, {});

        TVector agents {
            CreateAgentConfig("agent1", 1, "rack1"),
            CreateAgentConfig("agent2", 2, "rack2"),
            CreateAgentConfig("agent3", 3, "rack2")
        };

        // tag some devices with `ssd_local` (8 in total for each agents)
        for (auto& agent: agents) {
            int i = 0;
            for (auto& device: *agent.MutableDevices()) {
                if (i++ % 2 == 0) {
                    device.SetPoolName(ssdLocalTag);
                }
            }
        }

        for (const auto& agent: agents) {
            UNIT_ASSERT_VALUES_EQUAL(15, agent.DevicesSize());
            deviceList.UpdateDevices(agent);
        }

        auto allocate = [&] (ui32 n, ui32 nodeId, TString name) {
            return deviceList.AllocateDevices(
                Sprintf("disk-%d-%d-%s", n, nodeId, name.c_str()),
                {
                    .OtherRacks = {},
                    .LogicalBlockSize = DefaultBlockSize,
                    .BlockCount = n * DefaultBlockCount,
                    .PoolName = name,
                    .NodeIds = { nodeId }
                }
            );
        };

        // allocate one device from each agent and default pool
        for (const auto& agent: agents) {
            const auto nodeId = agent.GetNodeId();
            auto devices = allocate(1, nodeId, DefaultPoolName);
            UNIT_ASSERT_VALUES_EQUAL_C(1, devices.size(), agent.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(nodeId, devices[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(DefaultPoolName, devices[0].GetPoolName());
        }

        // allocate one device from each agent and ssd_local pool
        for (const auto& agent: agents) {
            const auto nodeId = agent.GetNodeId();
            auto devices = allocate(1, nodeId, ssdLocalTag);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(nodeId, devices[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(ssdLocalTag, devices[0].GetPoolName());
        }

        for (const auto& agent: agents) {
            auto devices = allocate(8, agent.GetNodeId(), ssdLocalTag);
            // can't allocate 8 devices
            UNIT_ASSERT_VALUES_EQUAL(0, devices.size());
        }

        for (int n: { 3, 4 }) {
            for (const auto& agent: agents) {
                auto devices = allocate(n, agent.GetNodeId(), ssdLocalTag);
                UNIT_ASSERT_VALUES_EQUAL(n, devices.size());
                for (auto& d: devices) {
                    UNIT_ASSERT_VALUES_EQUAL(agent.GetNodeId(), d.GetNodeId());
                    UNIT_ASSERT_VALUES_EQUAL(ssdLocalTag, d.GetPoolName());
                }
            }
        }
    }*/

    Y_UNIT_TEST(ShouldAllocateFromLocalPoolOnlyAtSingleHost)
    {
        const TString localPoolName = "local-ssd";

        TDeviceList deviceList({}, {});

        TVector agents {
            CreateAgentConfig("agent1", 1, "rack1"),
            CreateAgentConfig("agent2", 2, "rack2"),
            CreateAgentConfig("agent3", 3, "rack2")
        };

        for (auto& agent: agents) {
            auto& device = *agent.MutableDevices(0);
            device.SetPoolName(localPoolName);
            device.SetPoolKind(NProto::DEVICE_POOL_KIND_LOCAL);
        }

        for (const auto& agent: agents) {
            UNIT_ASSERT_VALUES_EQUAL(15, agent.DevicesSize());
            deviceList.UpdateDevices(agent);
        }

        auto allocate = [&] (ui32 n) {
            return deviceList.AllocateDevices(
                "disk-id",
                {
                    .OtherRacks = {},
                    .LogicalBlockSize = DefaultBlockSize,
                    .BlockCount = n * DefaultBlockCount,
                    .PoolKind = NProto::DEVICE_POOL_KIND_LOCAL,
                    .NodeIds = { 1, 2, 3 }
                }
            );
        };

        {
            auto devices = allocate(3);
            UNIT_ASSERT_VALUES_EQUAL(0, devices.size());
        }

        {
            auto devices = allocate(2);
            UNIT_ASSERT_VALUES_EQUAL(0, devices.size());
        }

        {
            auto devices = allocate(1);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(localPoolName, devices[0].GetPoolName());
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
