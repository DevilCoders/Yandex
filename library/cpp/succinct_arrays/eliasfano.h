#pragma once

#include "intvector.h"
#include "eliasfanomonotone.h"

#include <util/generic/bitops.h>
#include <util/generic/ylimits.h>
#include <util/generic/ptr.h>

namespace NSuccinctArrays {
    template <class TVal = ui64>
    class TEliasFanoArray {
    private:
        typedef TEliasFanoMonotoneArray<ui64> TLenCoder;
        TVal LowerBound_;
        TVal Offset_;
        ui64 PrevOffset_;
        TBitVector<ui64> Vals_;
        TLenCoder Lens_;
        size_t Size_;

    protected:
        TEliasFanoArray(const TVal& maxOffset, size_t length);
        TEliasFanoArray(const TVal* begin, const TVal* end, const TVal& lowerBound);
        void Learn(const TVal* begin, const TVal* end);
        void Learn(const TVal& maxOffset, size_t length);
        void Encode(const TVal* begin, const TVal* end, const TVal& lowerBound);
        void Encode(const TVal* begin, const TVal* end);
        void Add(const TVal& value);
        void Finish();

    public:
        TEliasFanoArray();
        TEliasFanoArray(const TVal* begin, const TVal* end);
        TVal Get(size_t pos) const;
        TVal operator[](size_t pos) const;
        void Save(IOutputStream* out) const;
        void Load(IInputStream* inp);
        ui64 Space() const;
        size_t size() const;
        void Swap(TEliasFanoArray<TVal>& TOther);
    };

    template <class TVal>
    TEliasFanoArray<TVal>::TEliasFanoArray()
        : LowerBound_(0)
        , Offset_(0)
        , PrevOffset_(0)
        , Vals_()
        , Lens_()
        , Size_(0)
    {
    }

    template <class TVal>
    TEliasFanoArray<TVal>::TEliasFanoArray(const TVal& maxOffset, size_t length)
        : LowerBound_(0)
        , Offset_(0)
        , PrevOffset_(0)
        , Vals_()
        , Lens_()
        , Size_(length)
    {
        Learn(maxOffset, length);
    }

    template <class TVal>
    TEliasFanoArray<TVal>::TEliasFanoArray(const TVal* begin, const TVal* end, const TVal& lowerBound)
        : LowerBound_(lowerBound)
        , Offset_(0)
        , PrevOffset_(0)
        , Vals_()
        , Lens_()
        , Size_(end - begin)
    {
        Encode(begin, end, lowerBound);
    }

    template <class TVal>
    TEliasFanoArray<TVal>::TEliasFanoArray(const TVal* begin, const TVal* end)
        : LowerBound_(0)
        , Offset_(0)
        , PrevOffset_(0)
        , Vals_()
        , Lens_()
        , Size_(end - begin)
    {
        Learn(begin, end);
        Encode(begin, end);
        Finish();
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Learn(const TVal* begin, const TVal* end) {
        Size_ = end - begin;
        PrevOffset_ = 0;
        LowerBound_ = Max<TVal>();
        for (const TVal* it = begin; it != end; ++it)
            LowerBound_ = Min<TVal>(*it, LowerBound_);
        Offset_ = LowerBound_ - 1;
        ui64 MaxOffset_ = 0;
        for (const TVal* it = begin; it != end; ++it)
            MaxOffset_ += MostSignificantBit(*it - Offset_);
        Lens_ = TLenCoder(MaxOffset_ + 1, end - begin + 1);
        Lens_.Add(0);
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Learn(const TVal& maxOffset, size_t length) {
        Size_ = length;
        PrevOffset_ = 0;
        LowerBound_ = 1;
        Offset_ = LowerBound_ - 1;
        ui64 MaxOffset_ = maxOffset;
        Lens_ = TLenCoder(MaxOffset_ + 1, length + 1);
        Lens_.Add(0);
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Encode(const TVal* begin, const TVal* end) {
        for (const TVal* it = begin; it != end; ++it) {
            Add(*it);
        }
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Encode(const TVal* begin, const TVal* end, const TVal& lowerBound) {
        LowerBound_ = lowerBound;
        Offset_ = LowerBound_ - 1;
        TVector<ui64> offsets;
        offsets.push_back(0);
        for (const TVal* it = begin; it != end; ++it) {
            ui64 value = *it;
            Y_ASSERT(value >= lowerBound);
            ui8 msb = MostSignificantBit(value - Offset_);
            Vals_.Append((value - Offset_) & (static_cast<TVal>(1) << msb) - 1, msb);
            offsets.push_back(offsets.back() + msb);
        }
        Lens_ = TLenCoder(offsets.begin(), offsets.end());
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Finish() {
        Lens_.Finish();
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Add(const TVal& value) {
        Y_ASSERT(value >= LowerBound_);
        ui8 msb = MostSignificantBit(value - Offset_);
        Vals_.Append((value - Offset_) & (static_cast<TVal>(1) << msb) - 1, msb);
        Lens_.Add(PrevOffset_ + msb);
        PrevOffset_ += msb;
    }

    template <class TVal>
    TVal TEliasFanoArray<TVal>::Get(size_t pos) const {
        ui64 from = Lens_.Get(pos);
        ui64 to = Lens_.Get(pos + 1);
        ui8 width = static_cast<ui8>(to - from);
        TVal value = static_cast<TVal>(Vals_.Get(static_cast<size_t>(from), width));
        value |= static_cast<TVal>(1) << width;
        return value + Offset_;
    }
    template <class TVal>
    TVal TEliasFanoArray<TVal>::operator[](size_t pos) const {
        return Get(pos);
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Save(IOutputStream* out) const {
        ::Save(out, LowerBound_);
        ::Save(out, Offset_);
        ::Save(out, PrevOffset_);
        ::SaveSize(out, Size_);
        ::Save(out, Vals_);
        ::Save(out, Lens_);
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Load(IInputStream* inp) {
        ::Load(inp, LowerBound_);
        ::Load(inp, Offset_);
        ::Load(inp, PrevOffset_);
        Size_ = ::LoadSize(inp);
        ::Load(inp, Vals_);
        ::Load(inp, Lens_);
    }

    template <class TVal>
    ui64 TEliasFanoArray<TVal>::Space() const {
        return (sizeof(LowerBound_) + sizeof(Offset_) + sizeof(PrevOffset_) + sizeof(Size_)) * CHAR_BIT +
               Vals_.Space() + Lens_.Space();
    }

    template <class TVal>
    size_t TEliasFanoArray<TVal>::size() const {
        return Size_;
    }

    template <class TVal>
    void TEliasFanoArray<TVal>::Swap(TEliasFanoArray<TVal>& other) {
        DoSwap(LowerBound_, other.LowerBound_);
        DoSwap(Offset_, other.Offset_);
        DoSwap(PrevOffset_, other.PrevOffset_);
        DoSwap(Vals_, other.Vals_);
        DoSwap(Lens_, other.Lens_);
        DoSwap(Size_, other.Size_);
    }

}
