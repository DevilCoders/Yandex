#pragma once

#include <util/memory/blob.h>

#include <library/cpp/offroad/utility/masks.h>

#include "flat_common.h"

namespace NOffroad {
/**
 * Quick and dirty implementation of `TFlatSearcher`-like reader for a packed
 * sequence of `ui32` integers.
 *
 * Use `TFlatWriter<ui32, nullptr_t, TUi32Vectorizer, TNullVectorizer>` to
 * write data for this searcher.
 */
class TFlatUi32Searcher {
public:

    TFlatUi32Searcher() = default;

    TFlatUi32Searcher(const TBlob& source) {
        Reset(source);
    }

    void Reset(const TBlob& source) {
        Source_ = source;
        Header_ = NPrivate::SelectBitsFromFlatHeader<0>(source);
        KeyMask_ = ScalarMask(Header_);
        Size_ = (source.Size() * 8 - 6) / Header_;
    }

    size_t Size() const {
        return Size_;
    }

    inline ui32 ReadKey(size_t index) const {
        Y_ASSERT(index < Size_);
        return NPrivate::LoadBits(Source_, 6 + Header_ * index, KeyMask_);
    }

    size_t FlatSize(size_t elements) {
        return (Header_ * elements + 13) / 8;
    }

private:
    TBlob Source_;
    size_t Size_ = 0;
    ui32 Header_ = 0;
    ui32 KeyMask_ = 0;
};

}
