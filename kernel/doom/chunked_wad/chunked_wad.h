#pragma once

#include "doc_chunk_mapping_searcher.h"

#include <kernel/doom/wad/wad.h>


namespace NDoom {


class IChunkedWad: public IWad {
public:
    static THolder<IChunkedWad> OpenChunked(
        const TString& prefix,
        bool lockMemory);

    static THolder<IChunkedWad> Open(const TString& indexPath, bool lockMemory = false);
    static THolder<IChunkedWad> Open(const TArrayRef<const char>& source);
    static THolder<IChunkedWad> Open(TBuffer&& buffer);

    virtual TVector<TWadLumpId> ChunkGlobalLumps(ui32 chunk) const = 0;

    virtual bool HasChunkGlobalLump(ui32 chunk, TWadLumpId type) const = 0;

    virtual TBlob LoadChunkGlobalLump(ui32 chunk, TWadLumpId type) const = 0;

    virtual ui32 Chunks() const = 0;

    virtual ui32 DocChunk(ui32 docId) const = 0;
};

} // namespace NDoom
