#pragma once

#include "blob_reader.h"
#include "intvector.h"
#include "rankselect.h"

#include <algorithm>

#include <util/generic/bitops.h>
#include <util/generic/ylimits.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>

namespace NSuccinctArrays {
    /*
Implementation of Elias-Fano representation of nondecreasing sequences.
Given the sequence x_0, x_1 ... x_n-1 where x_i <= x_i+1, x_i < u at most 2 + log(u/n) bits talen per element
Elias, Peter. "Efficient Storage and Retrieval by Content and Address of Static Files." Journal of the ACM 21, no. 2 (April 1974): 246-260.
doi:10.1145/321812.321820. http://portal.acm.org/citation.cfm?doid=321812.321820.
Fano, Rpbert. "On the number of bits required to implement an associative memory. Memorandum 61." Computer Structures Group, Project MAC, MIT, (1971).
http://scholar.google.com/scholar?hl=en&btnG=Search&q=intitle:On+the+Number+of+Bits+Required+to+Implement+An+Associative+Memory#2.

*/

    template <typename TVal>
    class TReadonlyEliasFanoMonotoneArray;

    template <class TVal = ui64>
    class TEliasFanoMonotoneArray {
    private:
        friend class TReadonlyEliasFanoMonotoneArray<TVal>;
        ui64 Counter_;
        ui64 UpperBound_;
        ui8 LowerBitsWidth_;
        TBitVector<ui64> UpperBits_;
        TIntVector LowerBits_;
        TRankSelect Select_;

    public:
        TEliasFanoMonotoneArray();
        TEliasFanoMonotoneArray(const TVal& upperBound, size_t length);
        TEliasFanoMonotoneArray(const TVal* begin, const TVal* end);
        void Learn(const TVal& upperBound, size_t length);
        void Learn(const TVal* begin, const TVal* end);
        void Encode(const TVal* begin, const TVal* end);
        void Finish();
        void Add(const TVal& value);
        TVal Get(size_t pos) const;
        TVal operator[](size_t pos) const;
        void Save(IOutputStream* out) const;
        void Load(IInputStream* inp);
        ui64 Space() const;
        void Clear();
        void Swap(TEliasFanoMonotoneArray<TVal>&);
        ui64 Size() const;
        ui64 size() const;
        bool Empty() const;
        bool empty() const;
    };

    template <typename TVal, typename TUpperBits, typename TLowerBits>
    TVal GetElement(
        const TRankSelect& select,
        const TUpperBits& upperBits,
        const TLowerBits& lowerBits,
        ui8 lowerBitsWidth,
        size_t pos) {
        return static_cast<TVal>((((select.Select(upperBits.Data(), pos) - pos) << lowerBitsWidth) | lowerBits.Get(pos)));
    }

    template <class TVal>
    TEliasFanoMonotoneArray<TVal>::TEliasFanoMonotoneArray()
        : Counter_(0)
        , UpperBound_(0)
        , LowerBitsWidth_(0)
        , UpperBits_()
        , LowerBits_()
        , Select_()
    {
    }

    template <class TVal>
    TEliasFanoMonotoneArray<TVal>::TEliasFanoMonotoneArray(const TVal* begin, const TVal* end)
        : Counter_(0)
        , UpperBound_(0)
        , LowerBitsWidth_(0)
    {
        Learn(begin, end);
        Encode(begin, end);
        Finish();
    }

    template <class TVal>
    TEliasFanoMonotoneArray<TVal>::TEliasFanoMonotoneArray(const TVal& upperBound, size_t length)
        : Counter_(0)
        , UpperBound_(upperBound)
        , LowerBitsWidth_(0)
    {
        Learn(upperBound, length);
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Learn(const TVal& upperBound, size_t length) {
        Counter_ = 0;
        UpperBound_ = upperBound;
        LowerBitsWidth_ = length ? Max<i8>(0, MostSignificantBit(UpperBound_ / length)) : 0;
        LowerBits_ = TIntVector(length, LowerBitsWidth_);
        UpperBits_ = TBitVector<ui64>(length + static_cast<size_t>(UpperBound_ >> LowerBitsWidth_) + 1);
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Learn(const TVal* begin, const TVal* end) {
        Learn(*std::max_element(begin, end) + 1, (size_t)(end - begin));
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Encode(const TVal* begin, const TVal* end) {
        ui64 prev = 0;
        for (const TVal* it = begin; it != end; ++it) {
            const ui64& value = *it;
            Y_ASSERT(value < UpperBound_);
            Y_ASSERT(value >= prev);
            Add(value);
            prev = value;
        }
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Finish() {
        if (Counter_)
            Select_ = TRankSelect(UpperBits_.Data(), UpperBits_.Size());
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Add(const TVal& value) {
        LowerBits_.Set(Counter_, value);
        UpperBits_.Set(static_cast<size_t>((value >> LowerBitsWidth_) + Counter_++));
    }

    template <class TVal>
    TVal TEliasFanoMonotoneArray<TVal>::Get(size_t pos) const {
        return GetElement<TVal>(Select_, UpperBits_, LowerBits_, LowerBitsWidth_, pos);
    }

    template <class TVal>
    TVal TEliasFanoMonotoneArray<TVal>::operator[](size_t pos) const {
        return Get(pos);
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Save(IOutputStream* out) const {
        ::Save(out, Counter_);
        ::Save(out, UpperBound_);
        ::Save(out, LowerBitsWidth_);

        if (Counter_) {
            ::Save(out, UpperBits_);
            ::Save(out, LowerBits_);
            ::Save(out, Select_);
        }
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Load(IInputStream* inp) {
        ::Load(inp, Counter_);
        ::Load(inp, UpperBound_);
        ::Load(inp, LowerBitsWidth_);

        if (Counter_) {
            ::Load(inp, UpperBits_);
            ::Load(inp, LowerBits_);
            ::Load(inp, Select_);
        }
    }

    template <class TVal>
    ui64 TEliasFanoMonotoneArray<TVal>::Space() const {
        const size_t sz = (sizeof(Counter_) + sizeof(UpperBound_) + sizeof(LowerBitsWidth_)) * CHAR_BIT;
        return sz + (Counter_ ? UpperBits_.Space() + LowerBits_.Space() + Select_.Space() : 0);
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Swap(TEliasFanoMonotoneArray<TVal>& a) {
        DoSwap(Counter_, a.Counter_);
        DoSwap(UpperBound_, a.UpperBound_);
        DoSwap(LowerBitsWidth_, a.LowerBitsWidth_);
        DoSwap(UpperBits_, a.UpperBits_);
        DoSwap(LowerBits_, a.LowerBits_);
        DoSwap(Select_, a.Select_);
    }

    template <class TVal>
    void TEliasFanoMonotoneArray<TVal>::Clear() {
        TEliasFanoMonotoneArray<TVal> a;
        Swap(a);
    }

    template <class TVal>
    ui64 TEliasFanoMonotoneArray<TVal>::Size() const {
        return Counter_;
    }

    template <class TVal>
    ui64 TEliasFanoMonotoneArray<TVal>::size() const {
        return Size();
    }

    template <class TVal>
    bool TEliasFanoMonotoneArray<TVal>::Empty() const {
        return !Size();
    }

    template <class TVal>
    bool TEliasFanoMonotoneArray<TVal>::empty() const {
        return !Size();
    }

    template <class TVal = ui64>
    class TReadonlyEliasFanoMonotoneArray {
    public:
        TReadonlyEliasFanoMonotoneArray()
            : Counter()
            , UpperBound()
            , LowerBitsWidth()
        {
        }

        static void SaveForReadonlyAccess(IOutputStream* out, const TEliasFanoMonotoneArray<TVal>& array) {
            ::Save(out, array.Counter_);
            ::Save(out, array.UpperBound_);
            ::Save(out, static_cast<ui64>(array.LowerBitsWidth_));
            if (array.Counter_) {
                TReadonlyBitVector<ui64>::SaveForReadonlyAccess(out, array.UpperBits_);
                TReadonlyIntVector::SaveForReadonlyAccess(out, array.LowerBits_);
                array.Select_.SaveForReadonlyAccess(out);
            }
        }

        TBlob LoadFromBlob(const TBlob& blob) {
            TBlobReader reader(blob);
            reader.ReadInteger(&Counter);
            reader.ReadInteger(&UpperBound);

            ui64 lowerBitsWidth{};
            reader.ReadInteger(&lowerBitsWidth);
            Y_ENSURE(
                lowerBitsWidth <= std::numeric_limits<ui8>::max(),
                "the lower bits width is too large; make sure you used SaveAligned() when saving data");
            LowerBitsWidth = static_cast<ui8>(lowerBitsWidth);

            auto tail = UpperBits.LoadFromBlob(reader.Tail());
            tail = LowerBits.LoadFromBlob(tail);
            return Select.LoadFromBlob(tail);
        }

        TVal Get(size_t pos) const {
            return GetElement<TVal>(Select, UpperBits, LowerBits, LowerBitsWidth, pos);
        }

        TVal Size() const {
            return Counter;
        }

        template <class TCont>
        void Enumerate(TCont&& cont) const {
            const auto upperSize = UpperBits.Size();
            TVal upper = 0;
            TVal index = 0;
            for (size_t i = 0; i != upperSize; ++i) {
                if (UpperBits.Test(i)) {
                    if (!cont(index, (upper << LowerBitsWidth) | LowerBits.Get(index))) {
                        break;
                    }
                    ++index;
                } else {
                    ++upper;
                }
            }
        }

    private:
        ui64 Counter;
        ui64 UpperBound;
        ui8 LowerBitsWidth;
        TReadonlyBitVector<ui64> UpperBits;
        TReadonlyIntVector LowerBits;
        TRankSelect Select;
    };

}
