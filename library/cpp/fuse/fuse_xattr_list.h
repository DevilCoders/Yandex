#pragma once

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

namespace NFuse {

    class TFuseXattrList {
    public:
        explicit TFuseXattrList(size_t maxSize)
            : MaxSize_(maxSize)
        {
        }

        bool Add(TStringBuf name);

        TStringBuf AsStrBuf() const {
            Y_ENSURE(ReadSize_ <= MaxSize_);
            return {Buf_.Get(), ReadSize_};
        }

        inline bool SizeRequested() const {
            return MaxSize_ == 0;
        }

        inline bool ResponseFitsSize() const {
            return MaxSize_ >= ReadSize_;
        }

        inline size_t GetRequiredSize() const {
            return ReadSize_;
        }

    private:
        TArrayHolder<char> Buf_;
        size_t MaxSize_;
        size_t ReadSize_ = 0;
    };

} // namespace NFuse
