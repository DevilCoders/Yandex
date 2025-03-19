#include "part_mirror.h"

#include "part_mirror_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateMirrorPartition(
    TStorageConfigPtr config,
    IProfileLogPtr profileLog,
    IBlockDigestGeneratorPtr digestGenerator,
    TString rwClientId,
    TNonreplicatedPartitionConfigPtr partConfig,
    TVector<TDevices> replicas,
    NRdma::IClientPtr rdmaClient)
{
    return std::make_unique<TMirrorPartitionActor>(
        std::move(config),
        std::move(profileLog),
        std::move(digestGenerator),
        std::move(rwClientId),
        std::move(partConfig),
        std::move(replicas),
        std::move(rdmaClient));
}

}   // namespace NCloud::NBlockStore::NStorage
