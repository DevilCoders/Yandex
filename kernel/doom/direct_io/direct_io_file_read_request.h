#pragma once

#include <util/generic/buffer.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/system/align.h>


namespace NDoom {


class TDirectIoFileReadRequest {
    static constexpr ui64 Alignment = 4096; // TODO(sankear): always alignment is 4096?
public:
    TDirectIoFileReadRequest() = default;

    TDirectIoFileReadRequest(ui64 offset, ui64 size) {
        AlignedOffset_ = AlignDown(offset, Alignment);

        const ui64 additionalSize = offset - AlignedOffset_;
        ExpandedSize_ = size + additionalSize;
        AlignedSize_ = AlignUp(ExpandedSize_, Alignment);
        Y_ENSURE(AlignedSize_ <= Max<ui32>() / 2);

        TBuffer buffer;
        buffer.Resize(AlignedSize_ + Alignment);
        AlignedBuffer_ = AlignUp(buffer.Data(), Alignment);

        const size_t start = (AlignedBuffer_ - buffer.Data()) + additionalSize;
        const size_t end = start + size;

        TBlob blob = TBlob::FromBuffer(buffer);
        Result_ = blob.SubBlob(start, end);
    }

    ui64 AlignedOffset() const {
        return AlignedOffset_;
    }

    ui64 AlignedSize() const {
        return AlignedSize_;
    }

    ui64 ExpandedSize() const {
        return ExpandedSize_;
    }

    char* AlignedBuffer() const {
        return AlignedBuffer_;
    }

    TBlob MoveResult() {
        return std::move(Result_);
    }

private:
    ui64 AlignedOffset_ = 0;
    ui64 AlignedSize_ = 0;
    ui64 ExpandedSize_ = 0;
    char* AlignedBuffer_ = nullptr;
    TBlob Result_;
};


} // namespace NDoom
