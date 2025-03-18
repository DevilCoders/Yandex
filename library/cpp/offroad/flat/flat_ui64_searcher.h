#pragma once

#include "flat_common.h"

#include <util/memory/blob.h>

namespace NOffroad {
/**
 * Quick and dirty implementation of `TFlatSearcher`-like reader for a packed
 * sequence of `ui64` integers.
 *
 * Use `TFlatWriter<ui64, nullptr_t, TUi64Vectorizer, TNullVectorizer>` to
 * write data for this searcher.
 */
    class TFlatUi64Searcher {
    public:
        TFlatUi64Searcher() = default;

        TFlatUi64Searcher(const TBlob& source) {
            Reset(source);
        }

        void Reset() {
            Reset(TBlob());
        }

        void Reset(const TBlob& source) {
            Source_ = source;

            if (source.Empty()) {
                Size_ = 0;
                return;
            }

            HiBits_ = NPrivate::SelectBitsFromFlatHeader<0>(source);
            LoBits_ = NPrivate::SelectBitsFromFlatHeader<1>(source);
            Y_ASSERT(LoBits_ <= 32);
            InvLoBits_ = 32 - LoBits_;
            KeyBits_ = HiBits_ + LoBits_;
            KeyMask_ = ScalarMask(KeyBits_);
            LoMask_ = ScalarMask(LoBits_);
            HiMask_ = ~ScalarMask(32);

            Y_ASSERT(source.Size() * 8 >= 12);
            Size_ = (source.Size() * 8 - 12) / KeyBits_;
        }

        size_t Size() const {
            return Size_;
        }

        inline ui64 ReadKey(size_t index) const {
            ui64 result = NPrivate::LoadBits(Source_, 12 + index * KeyBits_, KeyMask_);
            if (Y_LIKELY(HiBits_ == 0 || LoBits_ == 32)) {
                return result;
            }
            return (result & LoMask_) | ((result << InvLoBits_) & HiMask_);
        }

    private:
        TBlob Source_;
        size_t Size_ = 0;
        size_t HiBits_ = 0;
        size_t LoBits_ = 0;
        size_t InvLoBits_ = 0;
        ui64 KeyBits_ = 0;
        ui64 KeyMask_ = 0;
        ui64 LoMask_ = 0;
        ui64 HiMask_ = 0;
    };

}
