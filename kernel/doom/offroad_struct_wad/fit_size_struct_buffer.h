#pragma once
#include <util/generic/array_ref.h>
#include <util/generic/buffer.h>
#include <util/generic/yexception.h>

namespace NDoom {

class TFitSizeStructBuffer {
public:
    void Reset(ui32 indexStructSize, ui32 actualStructSize, const TArrayRef<const char>& region) {
        Y_ENSURE(indexStructSize <= actualStructSize);
        if (indexStructSize == actualStructSize) {
            Region_ = region;
            return;
        }
        Buffer_.Assign(~region, +region);
        Buffer_.Resize(actualStructSize);
        memset((char*)Buffer_.Data() + indexStructSize, 0, actualStructSize - indexStructSize);
        Region_ = TArrayRef<const char>(~Buffer_, +Buffer_);
    }

    TArrayRef<const char> Data() const {
        return Region_;
    }
private:
    TBuffer Buffer_;
    TArrayRef<const char> Region_;
};


} //namespace NDoom
