#include "agent_list.h"

#include <cloud/storage/core/libs/common/error.h>

#include <util/string/builder.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TByUUID
{
    template <typename T, typename U>
    bool operator () (const T& lhs, const U& rhs) const
    {
        return lhs.GetDeviceUUID() < rhs.GetDeviceUUID();
    }
};

template <typename T>
T SetDifference(const T& a, const T& b)
{
    T diff;

    std::set_difference(
        a.cbegin(), a.cend(),
        b.cbegin(), b.cend(),
        RepeatedPtrFieldBackInserter(&diff),
        TByUUID());

    return diff;
}

template <typename T>
T SetIntersection(const T& a, const T& b)
{
    T comm;

    std::set_intersection(
        a.cbegin(), a.cend(),
        b.cbegin(), b.cend(),
        RepeatedPtrFieldBackInserter(&comm),
        TByUUID());

    return comm;
}

NProto::TDeviceConfig& EnsureDevice(
    NProto::TAgentConfig& agent,
    const TString& uuid)
{
    auto& devices = *agent.MutableDevices();

    auto it = LowerBoundBy(
        devices.begin(),
        devices.end(),
        uuid,
        [] (const auto& x) -> const TString& {
            return x.GetDeviceUUID();
        });

    Y_VERIFY_DEBUG(it != devices.end());
    Y_VERIFY_DEBUG(it->GetDeviceUUID() == uuid);

    return *it;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TAgentList::TAgentList(
        NMonitoring::TDynamicCountersPtr counters,
        TVector<NProto::TAgentConfig> configs)
    : ComponentGroup(std::move(counters))
{
    Agents.reserve(configs.size());

    for (auto& config: configs) {
        AddAgent(std::move(config));
    }
}

NProto::TAgentConfig& TAgentList::AddAgent(NProto::TAgentConfig config)
{
    auto& agent = Agents.emplace_back(std::move(config));

    if (ComponentGroup) {
        Counters.emplace_back().Register(agent, ComponentGroup);
    }

    auto& devices = *agent.MutableDevices();

    Sort(devices, TByUUID());

    for (auto& device: devices) {
        device.SetNodeId(agent.GetNodeId());
        device.SetAgentId(agent.GetAgentId());
        if (device.GetUnadjustedBlockCount() == 0) {
            device.SetUnadjustedBlockCount(device.GetBlocksCount());
        }
    }

    const size_t agentIndex = Agents.size() - 1;

    AgentIdToIdx[agent.GetAgentId()] = agentIndex;

    if (const ui32 nodeId = agent.GetNodeId()) {
        NodeIdToIdx[nodeId] = agentIndex;
    }

    return agent;
}

NProto::TAgentConfig& TAgentList::AddNewAgent(
    NProto::TAgentConfig agentConfig,
    TInstant timestamp,
    THashSet<TDeviceId>* newDevices)
{
    Y_VERIFY(newDevices);

    agentConfig.SetStateTs(timestamp.MicroSeconds());

    for (auto& device: *agentConfig.MutableDevices()) {
        device.SetStateTs(timestamp.MicroSeconds());
        device.SetUnadjustedBlockCount(device.GetBlocksCount());

        newDevices->insert(device.GetDeviceUUID());
    }

    return AddAgent(std::move(agentConfig));
}

ui32 TAgentList::FindNodeId(const TAgentId& agentId) const
{
    const auto* agent = FindAgent(agentId);

    return agent
        ? agent->GetNodeId()
        : 0;
}

const NProto::TAgentConfig* TAgentList::FindAgent(TNodeId nodeId) const
{
    return const_cast<TAgentList*>(this)->FindAgent(nodeId);
}

const NProto::TAgentConfig* TAgentList::FindAgent(const TAgentId& agentId) const
{
    return const_cast<TAgentList*>(this)->FindAgent(agentId);
}

NProto::TAgentConfig* TAgentList::FindAgent(TNodeId nodeId)
{
    auto it = NodeIdToIdx.find(nodeId);
    if (it == NodeIdToIdx.end()) {
        return nullptr;
    }

    return &Agents[it->second];
}

NProto::TAgentConfig* TAgentList::FindAgent(const TAgentId& agentId)
{
    auto it = AgentIdToIdx.find(agentId);
    if (it == AgentIdToIdx.end()) {
        return nullptr;
    }

    return &Agents[it->second];
}

void TAgentList::TransferAgent(
    NProto::TAgentConfig& agent,
    TNodeId newNodeId)
{
    Y_VERIFY_DEBUG(newNodeId != 0);
    Y_VERIFY_DEBUG(!FindAgent(newNodeId));

    NodeIdToIdx.erase(agent.GetNodeId());
    NodeIdToIdx[newNodeId] = std::distance(&Agents[0], &agent);

    agent.SetNodeId(newNodeId);
}

NProto::TAgentConfig& TAgentList::RegisterAgent(
    NProto::TAgentConfig agentConfig,
    TInstant timestamp,
    THashSet<TDeviceId>* newDeviceIds)
{
    Y_VERIFY(newDeviceIds);

    auto* agent = FindAgent(agentConfig.GetAgentId());

    if (!agent) {
        return AddNewAgent(std::move(agentConfig), timestamp, newDeviceIds);
    }

    if (agent->GetNodeId() != agentConfig.GetNodeId()) {
        TransferAgent(*agent, agentConfig.GetNodeId());
    }

    agent->SetSeqNumber(agentConfig.GetSeqNumber());
    agent->SetDedicatedDiskAgent(agentConfig.GetDedicatedDiskAgent());

    auto& newList = *agentConfig.MutableDevices();
    Sort(newList, TByUUID());

    auto newDevices = SetDifference(newList, agent->GetDevices());
    auto oldDevices = SetIntersection(newList, agent->GetDevices());
    auto removedDevices = SetDifference(agent->GetDevices(), newList);

    for (const auto& config: removedDevices) {
        auto& device = EnsureDevice(*agent, config.GetDeviceUUID());

        device.SetNodeId(agent->GetNodeId());
        device.SetAgentId(agent->GetAgentId());
        if (device.GetState() != NProto::DEVICE_STATE_ERROR) {
            device.SetState(NProto::DEVICE_STATE_ERROR);
            device.SetStateTs(timestamp.MicroSeconds());
            device.SetStateMessage("lost");
        }
    }

    for (auto& config: oldDevices) {
        auto& device = EnsureDevice(*agent, config.GetDeviceUUID());

        if (device.GetUnadjustedBlockCount() == 0) {
            device.SetUnadjustedBlockCount(device.GetBlocksCount());
        }

        // update volatile fields
        device.SetBaseName(config.GetBaseName());
        device.SetTransportId(config.GetTransportId());
        device.SetNodeId(agent->GetNodeId());
        device.SetAgentId(agent->GetAgentId());
        device.SetRack(config.GetRack());
        device.MutableRdmaEndpoint()->CopyFrom(config.GetRdmaEndpoint());

        if (config.GetState() == NProto::DEVICE_STATE_ERROR) {
            device.SetState(config.GetState());
            device.SetStateTs(config.GetStateTs());
            device.SetStateMessage(config.GetStateMessage());
        } else if (device.GetBlockSize() != config.GetBlockSize()
            || device.GetUnadjustedBlockCount() != config.GetBlocksCount())
        {
            device.SetState(NProto::DEVICE_STATE_ERROR);
            device.SetStateTs(timestamp.MicroSeconds());
            device.SetStateMessage(TStringBuilder() <<
                "configuration changed: "
                    << device.GetBlockSize() << "x" << device.GetUnadjustedBlockCount()
                    << " -> "
                    << config.GetBlockSize() << "x" << config.GetBlocksCount()
            );
        }
    }

    for (auto& device: newDevices) {
        newDeviceIds->insert(device.GetDeviceUUID());

        device.SetStateTs(timestamp.MicroSeconds());
        device.SetUnadjustedBlockCount(device.GetBlocksCount());

        device.SetNodeId(agent->GetNodeId());
        device.SetAgentId(agent->GetAgentId());

        *agent->MutableDevices()->Add() = std::move(device);
    }

    Sort(*agent->MutableDevices(), TByUUID());

    return *agent;
}

void TAgentList::PublishCounters(TInstant now)
{
    for (auto& counter: Counters) {
        counter.Publish(now);
    }
}

void TAgentList::UpdateCounters(const NProto::TAgentStats& stats)
{
    // TODO: add AgentId to TAgentStats & use AgentIdToIdx (NBS-3280)
    auto it = NodeIdToIdx.find(stats.GetNodeId());
    if (it != NodeIdToIdx.end()) {
        Counters[it->second].Update(stats);
    }
}

bool TAgentList::RemoveAgent(TNodeId nodeId)
{
    auto it = NodeIdToIdx.find(nodeId);

    if (it == NodeIdToIdx.end()) {
        return false;
    }

    RemoveAgentByIdx(it->second);

    return true;
}

bool TAgentList::RemoveAgent(const TAgentId& agentId)
{
    auto it = AgentIdToIdx.find(agentId);

    if (it == AgentIdToIdx.end()) {
        return false;
    }

    RemoveAgentByIdx(it->second);

    return true;
}

bool TAgentList::RemoveAgentFromNode(TNodeId nodeId)
{
    return NodeIdToIdx.erase(nodeId) != 0;
}

void TAgentList::RemoveAgentByIdx(size_t index)
{
    if (index != Agents.size() - 1) {
        std::swap(Agents[index], Agents.back());

        if (!Counters.empty()) {
            std::swap(Counters[index], Counters.back());
        }

        auto& buddy = Agents[index];
        AgentIdToIdx[buddy.GetAgentId()] = index;

        if (const ui32 nodeId = buddy.GetNodeId()) {
            NodeIdToIdx[nodeId] = index;
        }
    }

    auto& agent = Agents.back();

    AgentIdToIdx.erase(agent.GetAgentId());
    NodeIdToIdx.erase(agent.GetNodeId());

    Agents.pop_back();

    if (!Counters.empty()) {
        Counters.pop_back();
    }
}

}   // namespace NCloud::NBlockStore::NStorage
