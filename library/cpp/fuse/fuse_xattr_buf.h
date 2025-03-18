#pragma once

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>

namespace NFuse {

    class TFuseXattrBuf {
    public:
        explicit TFuseXattrBuf(size_t maxSize)
            : MaxSize_(maxSize)
        {
        }

        inline void PutResponse(TStringBuf value) {
            Buf_ = value;
        }

        TStringBuf AsStrBuf() const {
            return {Buf_.Data(), Buf_.Size()};
        }

        inline bool SizeRequested() const {
            return MaxSize_ == 0;
        }

        inline bool ResponseFitsSize() const {
            return MaxSize_ >= Buf_.Size();
        }

        inline size_t GetRequiredSize() const {
            return Buf_.Size();
        }

    private:
        TStringBuf Buf_;
        size_t MaxSize_;
    };

} // namespace NFuse
