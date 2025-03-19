#pragma once

#include "public.h"

#include "protocol.h"
#include "verbs.h"

namespace NCloud::NBlockStore::NRdma {

////////////////////////////////////////////////////////////////////////////////

struct TBufferPoolStats
{
    size_t ActiveChunksCount = 0;
    size_t CustomChunksCount = 0;
    size_t FreeChunksCount = 0;
};

////////////////////////////////////////////////////////////////////////////////

class TBufferPool
{
    class TImpl;
    class TChunk;

private:
    std::unique_ptr<TImpl> Impl;

public:
    TBufferPool();
    ~TBufferPool();

    void Init(ibv_pd* pd, int flags);

    struct TBuffer : TBufferDesc
    {
        TChunk* Chunk;
    };

    TBuffer AcquireBuffer(size_t bytesCount, bool ignoreCache = false);
    void ReleaseBuffer(TBuffer& buffer);

    const TBufferPoolStats& GetStats() const;
};

using TPooledBuffer = TBufferPool::TBuffer;

}   // namespace NCloud::NBlockStore::NRdma
