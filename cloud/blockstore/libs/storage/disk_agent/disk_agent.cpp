#include "disk_agent.h"

#include "disk_agent_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateDiskAgent(
    TStorageConfigPtr config,
    TDiskAgentConfigPtr agentConfig,
    NSpdk::ISpdkEnvPtr spdk,
    ICachingAllocatorPtr allocator,
    IStorageProviderPtr storageProvider,
    IProfileLogPtr profileLog,
    IBlockDigestGeneratorPtr blockDigestGenerator,
    ILoggingServicePtr logging,
    NRdma::IServerPtr rdmaServer)
{
    return std::make_unique<TDiskAgentActor>(
        std::move(config),
        std::move(agentConfig),
        std::move(spdk),
        std::move(allocator),
        std::move(storageProvider),
        std::move(profileLog),
        std::move(blockDigestGenerator),
        std::move(logging),
        std::move(rdmaServer));
}

}   // namespace NCloud::NBlockStore::NStorage
