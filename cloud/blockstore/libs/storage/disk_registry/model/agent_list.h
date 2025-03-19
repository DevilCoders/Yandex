#pragma once

#include "public.h"

#include "agent_counters.h"

#include <cloud/blockstore/libs/storage/protos/disk.pb.h>
#include <cloud/storage/core/libs/common/error.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TAgentList
{
    using TAgentId = TString;
    using TDeviceId = TString;
    using TNodeId = ui32;

private:
    const NMonitoring::TDynamicCountersPtr ComponentGroup;

    TVector<NProto::TAgentConfig> Agents;
    TVector<TAgentCounters> Counters;

    THashMap<TAgentId, size_t> AgentIdToIdx;
    THashMap<TNodeId, size_t> NodeIdToIdx;

public:
    TAgentList(
        NMonitoring::TDynamicCountersPtr counters,
        TVector<NProto::TAgentConfig> configs);

    const TVector<NProto::TAgentConfig>& GetAgents() const
    {
        return Agents;
    }

    TNodeId FindNodeId(const TAgentId& agentId) const;

    NProto::TAgentConfig* FindAgent(TNodeId nodeId);
    const NProto::TAgentConfig* FindAgent(TNodeId nodeId) const;

    NProto::TAgentConfig* FindAgent(const TAgentId& agentId);
    const NProto::TAgentConfig* FindAgent(const TAgentId& agentId) const;

    NProto::TAgentConfig& RegisterAgent(
        NProto::TAgentConfig config,
        TInstant timestamp,
        THashSet<TDeviceId>* newDevices);

    bool RemoveAgent(TNodeId nodeId);
    bool RemoveAgent(const TAgentId& agentId);

    bool RemoveAgentFromNode(TNodeId nodeId);

    void PublishCounters(TInstant now);
    void UpdateCounters(const NProto::TAgentStats& stats);

private:
    NProto::TAgentConfig& AddAgent(NProto::TAgentConfig config);

    NProto::TAgentConfig& AddNewAgent(
        NProto::TAgentConfig config,
        TInstant timestamp,
        THashSet<TDeviceId>* newDevices);

    void TransferAgent(
        NProto::TAgentConfig& agent,
        TNodeId newNodeId);

    void RemoveAgentByIdx(size_t index);
};

}   // namespace NCloud::NBlockStore::NStorage
