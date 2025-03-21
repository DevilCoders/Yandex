#pragma once

#include "public.h"

#include <util/generic/size_literals.h>
#include <util/string/builder.h>
#include <util/system/align.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TByteRange
{
    const ui64 Offset;
    const ui64 Length;
    const ui32 BlockSize;

    TByteRange(ui64 offset, ui64 length, ui32 blockSize)
        : Offset(offset)
        , Length(length)
        , BlockSize(blockSize)
    {
        Y_VERIFY(BlockSize);
    }

    ui64 End() const
    {
        return Offset + Length;
    }

    ui64 UnalignedHeadLength() const
    {
        return Min(AlignUp<ui64>(Offset, BlockSize), End()) - Offset;
    }

    ui64 UnalignedTailOffset() const
    {
        auto alignedEnd = AlignDown<ui64>(End(), BlockSize);
        return alignedEnd < Offset ? End() : alignedEnd;
    }

    ui64 RelativeUnalignedTailOffset() const
    {
        return UnalignedTailOffset() - Offset;
    }

    ui64 UnalignedTailLength() const
    {
        return End() - UnalignedTailOffset();
    }

    bool IsAligned() const
    {
        return !UnalignedHeadLength() && !UnalignedTailLength();
    }

    ui64 AlignedBlockOffset(ui64 blockOffset) const
    {
        const auto offset = (FirstAlignedBlock() + blockOffset) * BlockSize;
        Y_VERIFY(offset + BlockSize <= End());
        return offset;
    }

    ui64 RelativeAlignedBlockOffset(ui64 blockOffset) const
    {
        return AlignedBlockOffset(blockOffset) - Offset;
    }

    ui64 FirstBlock() const
    {
        const auto alignedOffset = AlignDown<ui64>(Offset, BlockSize);
        return alignedOffset / BlockSize;
    }

    ui64 FirstAlignedBlock() const
    {
        const auto alignedOffset = AlignUp<ui64>(Offset, BlockSize);
        return alignedOffset / BlockSize;
    }

    ui64 LastBlock() const
    {
        const auto alignedOffset = AlignDown<ui64>(End() - 1, BlockSize);
        return alignedOffset / BlockSize;
    }

    ui64 BlockCount() const
    {
        return LastBlock() - FirstBlock() + 1;
    }

    ui64 AlignedBlockCount() const
    {
        const auto fa = FirstAlignedBlock();
        const auto l = LastBlock();
        if (fa > l) {
            return 0;
        }

        auto c = l - fa + 1;
        if (UnalignedTailLength()) {
            --c;
        }

        return c;
    }

    TByteRange AlignedSuperRange() const
    {
        return {
            AlignDown<ui64>(Offset, BlockSize),
            BlockCount() * BlockSize,
            BlockSize
        };
    }

    TByteRange Intersect(TByteRange other) const
    {
        auto offset = Max(Offset, other.Offset);
        auto end = Min(End(), other.End());

        if (end <= offset) {
            return Empty(BlockSize);
        }

        return {offset, end - offset, BlockSize};
    }

    bool Overlaps(TByteRange other) const
    {
        return Intersect(other).Length > 0;
    }

    TString Describe() const
    {
        return TStringBuilder() << "[" << Offset << ", " << End() << ")";
    }

    static TByteRange Empty(ui32 blockSize = 4_KB)
    {
        return {0, 0, blockSize};
    }

    static TByteRange BlockRange(ui64 blockIndex, ui32 blockSize)
    {
        return {blockIndex * blockSize, blockSize, blockSize};
    }
};

}   // namespace NCloud::NFileStore::NStorage
