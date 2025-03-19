#pragma once

#include <cloud/blockstore/libs/common/block_range.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TRequestsInFlight
{
private:
    struct TImpl;
    std::unique_ptr<TImpl> Impl;

public:
    TRequestsInFlight();
    ~TRequestsInFlight();

    bool TryAddRequest(ui64 requestId, TBlockRange64 blockRange);
    void RemoveRequest(ui64 requestId);

    size_t Size() const;
};

}   // namespace NCloud::NBlockStore::NStorage
