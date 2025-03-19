#include "agent_list.h"

#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TDeviceConfig CreateDevice(TString id, ui64 size)
{
    NProto::TDeviceConfig config;
    config.SetDeviceName("name-" + id);
    config.SetDeviceUUID(id);
    config.SetBlockSize(4_KB);
    config.SetBlocksCount(size / 4_KB);
    config.SetTransportId("transport-" + id);
    config.SetBaseName("base-name");
    config.SetRack("the-rack");
    config.MutableRdmaEndpoint()->SetHost("rdma-" + id);
    config.MutableRdmaEndpoint()->SetPort(10020);

    return config;
}

NProto::TDeviceConfig CreateDevice(TString id)
{
    return CreateDevice(std::move(id), 1_GB);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TAgentListTest)
{
    Y_UNIT_TEST(ShouldRegisterAgent)
    {
        TAgentList agentList {nullptr, {}};

        UNIT_ASSERT_VALUES_EQUAL(0, agentList.GetAgents().size());
        UNIT_ASSERT(agentList.FindAgent(42) == nullptr);
        UNIT_ASSERT(agentList.FindAgent("unknown") == nullptr);
        UNIT_ASSERT_VALUES_EQUAL(0, agentList.FindNodeId("unknown"));

        UNIT_ASSERT(!agentList.RemoveAgent("unknown"));
        UNIT_ASSERT(!agentList.RemoveAgent(42));

        NProto::TAgentConfig expectedConfig;

        expectedConfig.SetAgentId("foo");
        expectedConfig.SetNodeId(1000);
        *expectedConfig.AddDevices() = CreateDevice("uuid-1");
        *expectedConfig.AddDevices() = CreateDevice("uuid-2");
        *expectedConfig.AddDevices() = CreateDevice("uuid-3");

        {
            THashSet<TString> newDevices;
            auto& agent = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(1),
                &newDevices);

            UNIT_ASSERT_VALUES_EQUAL(agent.GetAgentId(), expectedConfig.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(agent.GetNodeId(), expectedConfig.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(agent.DevicesSize(), expectedConfig.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(3, newDevices.size());
        }

        auto* agent = agentList.FindAgent(expectedConfig.GetNodeId());

        UNIT_ASSERT(agent != nullptr);
        UNIT_ASSERT_EQUAL(agent, agentList.FindAgent(expectedConfig.GetAgentId()));

        UNIT_ASSERT_VALUES_EQUAL(
            expectedConfig.GetNodeId(),
            agentList.FindNodeId(expectedConfig.GetAgentId()));

        UNIT_ASSERT_VALUES_EQUAL(expectedConfig.DevicesSize(), agent->DevicesSize());
    }

    Y_UNIT_TEST(ShouldRegisterAgentAtNewNode)
    {
        NProto::TAgentConfig config;

        config.SetAgentId("foo");
        config.SetNodeId(1000);
        *config.AddDevices() = CreateDevice("uuid-1");
        *config.AddDevices() = CreateDevice("uuid-2");
        *config.AddDevices() = CreateDevice("uuid-3");

        TAgentList agentList {nullptr, {config}};

        UNIT_ASSERT_VALUES_EQUAL(1, agentList.GetAgents().size());

        {
            auto* agent = agentList.FindAgent(config.GetNodeId());

            UNIT_ASSERT(agent != nullptr);
            UNIT_ASSERT_EQUAL(agent, agentList.FindAgent(config.GetAgentId()));

            UNIT_ASSERT_VALUES_EQUAL(
                config.GetNodeId(),
                agentList.FindNodeId(config.GetAgentId()));
        }

        config.SetNodeId(2000);

        THashSet<TString> newDevices;
        agentList.RegisterAgent(config, TInstant::FromValue(1), &newDevices);

        UNIT_ASSERT_VALUES_EQUAL(0, newDevices.size());
        UNIT_ASSERT_VALUES_EQUAL(1, agentList.GetAgents().size());

        {
            auto* agent = agentList.FindAgent(config.GetNodeId());

            UNIT_ASSERT(agent != nullptr);
            UNIT_ASSERT_EQUAL(agent, agentList.FindAgent(config.GetAgentId()));

            UNIT_ASSERT_VALUES_EQUAL(
                config.GetNodeId(),
                agentList.FindNodeId(config.GetAgentId()));
        }
    }

    Y_UNIT_TEST(ShouldKeepRegistryDeviceFieldsUponAgentReRegistration)
    {
        TAgentList agentList {nullptr, {}};

        NProto::TAgentConfig expectedConfig;

        expectedConfig.SetAgentId("foo");
        expectedConfig.SetNodeId(1000);
        *expectedConfig.AddDevices() = CreateDevice("uuid-1", 2_GB);

        {
            THashSet<TString> newDevices;
            auto& agent = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(1),
                &newDevices);

            UNIT_ASSERT_VALUES_EQUAL(agent.GetAgentId(), expectedConfig.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(agent.GetNodeId(), expectedConfig.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(agent.DevicesSize(), expectedConfig.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(1, newDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(
                "name-uuid-1",
                agent.GetDevices(0).GetDeviceName()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                agent.GetDevices(0).GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                2_GB / 4_KB,
                agent.GetDevices(0).GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                2_GB / 4_KB,
                agent.GetDevices(0).GetUnadjustedBlockCount()
            );

            agent.MutableDevices(0)->SetBlocksCount(1_GB / 4_KB);
            // agent.MutableDevices(0)->SetState(NProto::DEVICE_STATE_WARNING);
            agent.MutableDevices(0)->SetStateTs(111);
            agent.MutableDevices(0)->SetCmsTs(222);
            agent.MutableDevices(0)->SetStateMessage("the-message");
        }

        {
            THashSet<TString> newDevices;
            auto& agent = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(2),
                &newDevices);

            UNIT_ASSERT_VALUES_EQUAL(agent.GetAgentId(), expectedConfig.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(agent.GetNodeId(), expectedConfig.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(agent.DevicesSize(), expectedConfig.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, newDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                agent.GetDevices(0).GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                agent.GetDevices(0).GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                1_GB / 4_KB,
                agent.GetDevices(0).GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                2_GB / 4_KB,
                agent.GetDevices(0).GetUnadjustedBlockCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                111,
                agent.GetDevices(0).GetStateTs()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                222,
                agent.GetDevices(0).GetCmsTs()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "the-message",
                agent.GetDevices(0).GetStateMessage()
            );

            // these fields should be taken from agent's data
            UNIT_ASSERT_VALUES_EQUAL("the-rack", agent.GetDevices(0).GetRack());
            UNIT_ASSERT_VALUES_EQUAL(
                "base-name",
                agent.GetDevices(0).GetBaseName()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "transport-uuid-1",
                agent.GetDevices(0).GetTransportId()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "rdma-uuid-1",
                agent.GetDevices(0).GetRdmaEndpoint().GetHost()
            );
        }
    }

    Y_UNIT_TEST(ShouldUpdateDevices)
    {
        TAgentList agentList {nullptr, []{
            NProto::TAgentConfig foo;

            foo.SetAgentId("foo");
            foo.SetNodeId(1000);
            *foo.AddDevices() = CreateDevice("x");
            *foo.AddDevices() = CreateDevice("y");

            return TVector{foo};
        }()};

        {
            auto* foo = agentList.FindAgent("foo");
            UNIT_ASSERT_VALUES_UNEQUAL(nullptr, foo);
            UNIT_ASSERT_VALUES_EQUAL(foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo->GetState());
            UNIT_ASSERT_VALUES_EQUAL(2, foo->DevicesSize());

            auto& x = foo->GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, x.GetState());
            UNIT_ASSERT(x.GetStateMessage().empty());

            auto& y = foo->GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, y.GetState());
            UNIT_ASSERT(y.GetStateMessage().empty());
        }

        {
            THashSet<TString> newDevices;
            auto& foo = agentList.RegisterAgent([]{
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    *foo.AddDevices() = CreateDevice("x");
                    *foo.AddDevices() = CreateDevice("z");

                    return foo;
                }(),
                TInstant::FromValue(42),
                &newDevices);

            UNIT_ASSERT_VALUES_EQUAL(1, newDevices.size());
            UNIT_ASSERT_VALUES_EQUAL("z", *newDevices.begin());
            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(3, foo.DevicesSize());

            auto& x = *foo.MutableDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, x.GetState());
            UNIT_ASSERT(x.GetStateMessage().empty());
            x.SetState(NProto::DEVICE_STATE_WARNING);

            auto& y = foo.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());
            UNIT_ASSERT(!y.GetStateMessage().empty());

            auto& z = foo.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("z", z.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, z.GetState());
            UNIT_ASSERT(z.GetStateMessage().empty());
        }

        {
            THashSet<TString> newDevices;
            auto& foo = agentList.RegisterAgent([]{
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    *foo.AddDevices() = CreateDevice("x");
                    *foo.AddDevices() = CreateDevice("y");
                    *foo.AddDevices() = CreateDevice("z");

                    return foo;
                }(),
                TInstant::FromValue(42),
                &newDevices);

            UNIT_ASSERT_VALUES_EQUAL(0, newDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(3, foo.DevicesSize());

            auto& x = foo.GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_WARNING, x.GetState());
            UNIT_ASSERT(x.GetStateMessage().empty());

            auto& y = foo.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            // can't change state to online automatically
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());
            UNIT_ASSERT(!y.GetStateMessage().empty());

            auto& z = foo.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("z", z.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, z.GetState());
            UNIT_ASSERT(z.GetStateMessage().empty());
        }

        {
            THashSet<TString> newDevices;
            auto& foo = agentList.RegisterAgent([]{
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    auto& z = *foo.AddDevices();
                    z = CreateDevice("z");
                    z.SetBlockSize(512);

                    return foo;
                }(),
                TInstant::FromValue(42),
                &newDevices);

            UNIT_ASSERT_VALUES_EQUAL(0, newDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(3, foo.DevicesSize());

            auto& x = foo.GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, x.GetState());
            UNIT_ASSERT(!x.GetStateMessage().empty());

            auto& y = foo.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());
            UNIT_ASSERT(!y.GetStateMessage().empty());

            auto& z = foo.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("z", z.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, z.GetState());
            UNIT_ASSERT(!z.GetStateMessage().empty());
        }
    }

    Y_UNIT_TEST(ShouldUpdateCounters)
    {
        NProto::TAgentConfig foo;

        foo.SetAgentId("foo");
        foo.SetNodeId(1000);
        *foo.AddDevices() = CreateDevice("uuid-1");
        *foo.AddDevices() = CreateDevice("uuid-2");

        NProto::TAgentConfig bar;

        bar.SetAgentId("bar");
        bar.SetNodeId(2000);
        *bar.AddDevices() = CreateDevice("uuid-3");
        *bar.AddDevices() = CreateDevice("uuid-4");

        auto monitoring = CreateMonitoringServiceStub();
        auto diskRegistryGroup = monitoring->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");

        TAgentList agentList {diskRegistryGroup, {foo, bar}};

        auto fooCounters = diskRegistryGroup->GetSubgroup("agent", foo.GetAgentId());
        auto barCounters = diskRegistryGroup->GetSubgroup("agent", bar.GetAgentId());

        auto uuid1Counters = fooCounters->GetSubgroup("device", "uuid-1");
        auto uuid2Counters = fooCounters->GetSubgroup("device", "uuid-2");

        auto uuid3Counters = barCounters->GetSubgroup("device", "uuid-3");
        auto uuid4Counters = barCounters->GetSubgroup("device", "uuid-4");

        auto uuid1ReadCount = uuid1Counters->GetCounter("ReadCount");
        auto uuid1WriteCount = uuid1Counters->GetCounter("WriteCount");

        auto uuid2ReadCount = uuid2Counters->GetCounter("ReadCount");
        auto uuid2WriteCount = uuid2Counters->GetCounter("WriteCount");

        auto uuid3ReadCount = uuid3Counters->GetCounter("ReadCount");
        auto uuid3WriteCount = uuid3Counters->GetCounter("WriteCount");

        auto uuid4ReadCount = uuid4Counters->GetCounter("ReadCount");
        auto uuid4WriteCount = uuid4Counters->GetCounter("WriteCount");

        UNIT_ASSERT_VALUES_EQUAL(0, uuid1ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid1WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, uuid2ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid2WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, uuid3ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid3WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, uuid4ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid4WriteCount->Val());

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(1000);

            auto* uuid1Stats = stats.AddDeviceStats();
            uuid1Stats->SetDeviceUUID("uuid-1");
            uuid1Stats->SetNumReadOps(10);
            uuid1Stats->SetNumWriteOps(20);

            auto* uuid2Stats = stats.AddDeviceStats();
            uuid2Stats->SetDeviceUUID("uuid-2");
            uuid2Stats->SetNumReadOps(30);
            uuid2Stats->SetNumWriteOps(40);

            return stats;
        }());

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(2000);

            auto* uuid3Stats = stats.AddDeviceStats();
            uuid3Stats->SetDeviceUUID("uuid-3");
            uuid3Stats->SetNumReadOps(100);
            uuid3Stats->SetNumWriteOps(200);

            auto* uuid4Stats = stats.AddDeviceStats();
            uuid4Stats->SetDeviceUUID("uuid-4");
            uuid4Stats->SetNumReadOps(300);
            uuid4Stats->SetNumWriteOps(400);

            return stats;
        }());

        agentList.PublishCounters(TInstant::Hours(1));

        UNIT_ASSERT_VALUES_EQUAL(10, uuid1ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(20, uuid1WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(30, uuid2ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(40, uuid2WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(100, uuid3ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(200, uuid3WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(300, uuid4ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(400, uuid4WriteCount->Val());

        agentList.RemoveAgent(1000);

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(1000);

            auto* uuid1Stats = stats.AddDeviceStats();
            uuid1Stats->SetDeviceUUID("uuid-1");
            uuid1Stats->SetNumReadOps(1000);
            uuid1Stats->SetNumWriteOps(1000);

            auto* uuid2Stats = stats.AddDeviceStats();
            uuid2Stats->SetDeviceUUID("uuid-2");
            uuid2Stats->SetNumReadOps(1000);
            uuid2Stats->SetNumWriteOps(1000);

            return stats;
        }());

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(2000);

            auto* uuid3Stats = stats.AddDeviceStats();
            uuid3Stats->SetDeviceUUID("uuid-3");
            uuid3Stats->SetNumReadOps(1000);
            uuid3Stats->SetNumWriteOps(1000);

            auto* uuid4Stats = stats.AddDeviceStats();
            uuid4Stats->SetDeviceUUID("uuid-4");
            uuid4Stats->SetNumReadOps(1000);
            uuid4Stats->SetNumWriteOps(1000);

            return stats;
        }());

        agentList.PublishCounters(TInstant::Hours(2));

        UNIT_ASSERT_VALUES_EQUAL(10, uuid1ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(20, uuid1WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(30, uuid2ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(40, uuid2WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(1100, uuid3ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(1200, uuid3WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(1300, uuid4ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(1400, uuid4WriteCount->Val());
    }

    Y_UNIT_TEST(ShouldUpdateSeqNumber)
    {
        TAgentList agentList {nullptr, []{
            NProto::TAgentConfig foo;

            foo.SetAgentId("foo");
            foo.SetNodeId(1000);
            foo.SetSeqNumber(23);
            foo.SetDedicatedDiskAgent(false);
            *foo.AddDevices() = CreateDevice("uuid-1");
            *foo.AddDevices() = CreateDevice("uuid-2");

            return TVector{foo};
        }()};

        {
            auto* foo = agentList.FindAgent("foo");
            UNIT_ASSERT_VALUES_UNEQUAL(nullptr, foo);
            UNIT_ASSERT_VALUES_EQUAL(foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo->GetState());
            UNIT_ASSERT_VALUES_EQUAL(23, foo->GetSeqNumber());
            UNIT_ASSERT(!foo->GetDedicatedDiskAgent());
            UNIT_ASSERT_VALUES_EQUAL(2, foo->DevicesSize());
        }

        {
            THashSet<TString> newDevices;
            auto& foo = agentList.RegisterAgent([]{
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    foo.SetSeqNumber(27);
                    foo.SetDedicatedDiskAgent(true);
                    *foo.AddDevices() = CreateDevice("uuid-1");
                    *foo.AddDevices() = CreateDevice("uuid-2");

                    return foo;
                }(),
                TInstant::FromValue(42),
                &newDevices);

            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(27, foo.GetSeqNumber());
            UNIT_ASSERT(foo.GetDedicatedDiskAgent());
            UNIT_ASSERT_VALUES_EQUAL(2, foo.DevicesSize());
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
