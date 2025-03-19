#include "checkpoint.h"

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

void TCheckpointStore::Add(const TString& checkpointId)
{
    Checkpoints.emplace(checkpointId, false);
}

void TCheckpointStore::DeleteData(const TString& checkpointId)
{
    auto it = Checkpoints.FindPtr(checkpointId);
    if (it) {
        *it = true;
    }
}

void TCheckpointStore::Delete(const TString& checkpointId)
{
    Checkpoints.erase(checkpointId);
}

size_t TCheckpointStore::Size() const
{
    return Checkpoints.size();
}

TVector<TString> TCheckpointStore::Get() const
{
    TVector<TString> checkpoints(Reserve(Checkpoints.size()));
    for (const auto& [checkpoint, dataDeleted]: Checkpoints) {
        if (!dataDeleted) {
            checkpoints.push_back(checkpoint);
        }
    }
    return checkpoints;
}

const THashMap<TString, bool>& TCheckpointStore::GetAll() const
{
    return Checkpoints;
}

}   // namespace NCloud::NBlockStore::NStorage
