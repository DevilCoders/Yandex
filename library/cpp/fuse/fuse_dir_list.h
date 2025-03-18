#pragma once

#include "base.h"
#include "fuse_kernel.h"

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>

namespace NFuse {

class TFuseDirList {
public:
    explicit TFuseDirList()
        : Buf_(0)
    {
    }

    explicit TFuseDirList(size_t maxSize)
        : MaxSize_(maxSize)
    {
    }

    // Returns true when buffer has enough space and entry was added,
    // false otherwise.
    bool Add(TStringBuf name, TInodeNum ino, ui32 type, off_t off);

    TStringBuf AsStrBuf() const {
        return {Buf_.Data(), Buf_.Size()};
    }

private:
    TBuffer Buf_;
    size_t MaxSize_;
};

class TFuseDirListPlus {
public:
    explicit TFuseDirListPlus()
        : Buf_(0)
    {
    }

    explicit TFuseDirListPlus(size_t maxSize)
        : MaxSize_(maxSize)
    {
    }

    class TEntry {
    public:
        operator bool() const {
            return P_ != nullptr;
        }

        void Fill(const fuse_entry_out& entry);
    private:
        friend TFuseDirListPlus;
        void* P_ = nullptr;
    };

    TEntry Add(TStringBuf name, TInodeNum ino, ui32 type, off_t off);

    TStringBuf AsStrBuf() const {
        return {Buf_.Data(), Buf_.Size()};
    }

private:
    TBuffer Buf_;
    size_t MaxSize_;

};

} // namespace NFuse
