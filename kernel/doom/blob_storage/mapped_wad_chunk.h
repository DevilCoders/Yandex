#pragma once

#include "wad_chunk.h"


namespace NDoom {


class TMappedWadChunkDataReader {
public:
    TMappedWadChunkDataReader() = default;

    TMappedWadChunkDataReader(const TArrayRef<const char>& source)
        : Data_(TBlob::NoCopy(source.data(), source.size()))
    {

    }

    TMappedWadChunkDataReader(const TBlob& blob)
        : Data_(blob)
    {
    }

    TMappedWadChunkDataReader(const TString& path, bool lockMemory = false)
        : Data_(lockMemory ? TBlob::LockedFromFile(path) : TBlob::FromFile(path))
    {
    }

    ui64 Size() const {
        return Data_.Size();
    }

    TBlob Read(ui64 offset, ui64 size) const {
        Y_ASSERT(offset <= Data_.Size() && size <= Data_.Size() - offset);
        return TBlob::NoCopy(Data_.AsCharPtr() + offset, size);
    }

    TBlob ReadHead(ui64 offset, ui64 size) const {
        return Read(offset, size);
    }

    TBlob ReadSignature(ui64 offset, ui64 size) const {
        return Read(offset, size);
    }

    TBlob ReadSubSize(ui64 offset, ui64 size) const {
        return Read(offset, size);
    }

    TBlob ReadSub(ui64 offset, ui64 size) const {
        return Read(offset, size);
    }

private:
    TBlob Data_;
};


using TMappedWadChunk = TWadChunk<TMappedWadChunkDataReader>;


} // namespace NDoom
