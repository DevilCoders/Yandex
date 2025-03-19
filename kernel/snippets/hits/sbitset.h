#pragma once

#include <util/system/defaults.h>


namespace NSnippets
{
    // Bit-based static set for short sets of small numbers
    // with built-in iterator
    template <class TValue>
    class TShortBitSet {
    public:
        static const size_t MAX_BITS = 8 * sizeof(TValue);
        TShortBitSet()
            : Bits(0)
            , Count(0)
            , BitCount(0)
            , Mask(0)
            , Iterator(0)
        {}
        TShortBitSet(const TValue& bits)
            : Bits(bits)
            , Count(0)
            , BitCount(0)
            , Mask(0)
            , Iterator(0)
        {
            TValue mask;
            for (size_t i = 0; i < MAX_BITS; ++i) {
                mask = 1 << i;
                if (mask & Bits) {
                    Count += 1;
                    BitCount = i + 1;
                }
            }
        }
        inline TValue operator & (TValue mask) const {
            return Bits & mask;
        }
        // Sets specified element
        inline void Insert(TValue bit) {
            TValue mask = 1 << bit;
            if (Bits & mask) {
                return;
            }
            Bits |= mask;
            Count++;
            if (BitCount <= bit) {
                BitCount = bit + 1;
            }
        }
        // Clears and sets specified element
        inline void Reset(TValue bit) {
            Bits = 1 << bit;
            Count = 1;
            BitCount = bit + 1;
        }
        inline size_t Size() const {
            return Count;
        }
        // Iterator functions
        inline const size_t& Begin() {
            Mask = 1;
            Iterator = 0;
            while (!End() && !(Bits & Mask)) {
                ++Iterator;
                Mask <<= 1;
            }
            return Iterator;
        }
        inline bool End() {
            return Iterator >= BitCount;
        }
        inline void Next() {
            do {
                ++Iterator;
                Mask <<= 1;
            } while (!End() && !(Bits & Mask));
        }
        inline const TValue& GetBits() const {
            return Bits;
        }

        bool Equals(const TShortBitSet<TValue>& other) {
            return
                other.Bits == other.Bits &&
                other.Count == Count &&
                other.BitCount == BitCount;
        }

    private:
        TValue Bits;
        size_t Count;
        size_t BitCount;

        TValue Mask; // iterator mask
        size_t Iterator;
    };
}

