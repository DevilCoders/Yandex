#pragma once

#include "wad_chunk.h"

#include <kernel/doom/direct_io/direct_io_file_read_request.h>

#include <util/system/filemap.h>
#include <util/system/mlock.h>


namespace NDoom {


class TDirectIoWadChunkDataReader {
public:
    TDirectIoWadChunkDataReader(const TString& path, bool lockMemory = false)
        : File_(path, EOpenModeFlag::OpenExisting | EOpenModeFlag::RdOnly | EOpenModeFlag::Direct | EOpenModeFlag::DirectAligned)
        , Map_(File_, TMemoryMap::EOpenModeFlag::oRdOnly | TMemoryMap::EOpenModeFlag::oNotGreedy)
        , LockMemory_(lockMemory)
    {
        File_.SetDirect();
    }

    ~TDirectIoWadChunkDataReader() {
        if (LockMemory_ && Map_.MappedSize()) {
            UnlockMemory(Map_.Ptr(), Map_.MappedSize());
        }
    }

    FHANDLE FileHandle() const {
        return File_.GetHandle();
    }

    ui64 Size() const {
        const i64 length = File_.GetLength();
        Y_ASSERT(length >= 0);
        return length;
    }

    TBlob Read(ui64 offset, ui64 size) const {
        Y_ASSERT(offset <= Size() && size <= Size() - offset);
        TDirectIoFileReadRequest request(offset, size);
        i32 reallyRead = File_.RawPread(request.AlignedBuffer(), request.AlignedSize(), request.AlignedOffset());
        Y_ENSURE(reallyRead >= 0 && static_cast<ui32>(reallyRead) >= request.ExpandedSize());
        return request.MoveResult();
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

    TBlob ReadSub(ui64 offset, ui64 size) {
        Y_ASSERT(offset <= Size() && size <= Size() - offset);
        if (size == 0) {
            return TBlob();
        }
        Y_ENSURE(Map_.MappedSize() == 0);
        Map_.Map(offset, size);
        Y_ENSURE(Map_.MappedSize() == size);
        Y_ENSURE(Map_.Ptr());
        if (LockMemory_) {
            LockMemory(Map_.Ptr(), size);
        }

        return TBlob::NoCopy(Map_.Ptr(), size);
    }

private:
    TFile File_;
    TFileMap Map_;
    bool LockMemory_ = false;
};


class TDirectIoWadChunk: public TWadChunk<TDirectIoWadChunkDataReader> {
    using TBase = TWadChunk<TDirectIoWadChunkDataReader>;
public:
    using TBase::TBase;

    FHANDLE FileHandle() const {
        return DataReader().FileHandle();
    }
};


} // namespace NDoom
