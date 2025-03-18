#pragma once

// Fredriksson, Kimmo, and Fedor Nikitin. "Simple Random Access Compression."
// Fundamenta Informaticae 92, no. 1-2 (2009): 63�81.
// http://portal.acm.org/citation.cfm?id=1551891.1551895
// Section 5.

#include "blob_reader.h"
#include "eliasfanomonotone.h"

#include <util/system/defaults.h>
#include <util/generic/bitops.h>
#include <util/generic/vector.h>

namespace NSuccinctArrays {
    template <typename TVal>
    class TReadonlyFredrikssonNikitinArray;

    template <class TVal>
    class TFredrikssonNikitinArray {
        friend class TReadonlyFredrikssonNikitinArray<TVal>;
        typedef TEliasFanoMonotoneArray<ui64> TLenCoder;
        ui64 PrevOffset_;
        THolder<TBitVector<ui64>> Vals_;
        THolder<TLenCoder> Lens_;

    public:
        TFredrikssonNikitinArray();
        TFredrikssonNikitinArray(const TVal* begin, const TVal* end);
        void Learn(const TVal* begin, const TVal* end);
        void Encode(const TVal* begin, const TVal* end);
        void Finish();
        void Add(const TVal& value);
        TVal Get(size_t pos) const;
        TVal operator[](size_t pos) const;
        void Save(IOutputStream* out) const;
        void Load(IInputStream* inp);
        ui64 Space() const;
    };

    template <typename TVal, typename TVals, typename TLens>
    TVal GetElement(const TVals& vals, const TLens& lens, size_t pos) {
        ui64 from = lens.Get(pos);
        ui64 to = lens.Get(pos + 1);
        ui8 width = static_cast<ui8>(to - from);
        ui64 value = vals.Get((size_t)from, (ui8)(to - from));
        return static_cast<TVal>(value - 2 + (static_cast<ui64>(1) << width));
    }

    template <class TVal>
    TFredrikssonNikitinArray<TVal>::TFredrikssonNikitinArray()
        : PrevOffset_(0)
        , Vals_(new TBitVector<ui64>())
        , Lens_(new TEliasFanoMonotoneArray<TVal>())
    {
    }

    template <class TVal>
    TFredrikssonNikitinArray<TVal>::TFredrikssonNikitinArray(const TVal* begin, const TVal* end)
        : PrevOffset_(0)
        , Vals_(new TBitVector<ui64>())
        , Lens_(new TLenCoder())
    {
        Learn(begin, end);
        Encode(begin, end);
        Finish();
    }

    template <class TVal>
    void TFredrikssonNikitinArray<TVal>::Learn(const TVal* begin, const TVal* end) {
        PrevOffset_ = 0;
        TVal MaxOffset_ = 0;
        for (const TVal* it = begin; it != end; ++it)
            MaxOffset_ += MostSignificantBit(*it + 2);
        Vals_.Reset(new TBitVector<ui64>());
        Lens_.Reset(new TLenCoder(MaxOffset_ + 1, end - begin + 1));
        Lens_->Add(0);
    }

    template <class TVal>
    void TFredrikssonNikitinArray<TVal>::Encode(const TVal* begin, const TVal* end) {
        for (const TVal* it = begin; it != end; ++it)
            Add(*it);
    }

    template <class TVal>
    void TFredrikssonNikitinArray<TVal>::Finish() {
        Lens_->Finish();
    }

    template <class TVal>
    void TFredrikssonNikitinArray<TVal>::Add(const TVal& value) {
        ui8 msb = MostSignificantBit(value + 2);
        Vals_->Append(value + 2 - (static_cast<ui64>(1) << msb), msb);
        Lens_->Add(PrevOffset_ + msb);
        PrevOffset_ += msb;
    }

    template <class TVal>
    TVal TFredrikssonNikitinArray<TVal>::operator[](size_t pos) const {
        return Get(pos);
    }

    template <class TVal>
    TVal TFredrikssonNikitinArray<TVal>::Get(size_t pos) const {
        return GetElement<TVal>(*Vals_, *Lens_, pos);
    }

    template <class TVal>
    void TFredrikssonNikitinArray<TVal>::Save(IOutputStream* out) const {
        ::Save(out, *Vals_);
        ::Save(out, *Lens_);
    }

    template <class TVal>
    void TFredrikssonNikitinArray<TVal>::Load(IInputStream* inp) {
        ::Load(inp, *Vals_);
        ::Load(inp, *Lens_);
    }

    template <class TVal>
    ui64 TFredrikssonNikitinArray<TVal>::Space() const {
        return Vals_->Space() + Lens_->Space();
    }

    template <typename TVal>
    class TReadonlyFredrikssonNikitinArray {
    public:
        TReadonlyFredrikssonNikitinArray() = default;

        static void SaveForReadonlyAccess(IOutputStream* out, const TFredrikssonNikitinArray<TVal>& array) {
            TReadonlyBitVector<ui64>::SaveForReadonlyAccess(out, *array.Vals_);
            TReadonlyEliasFanoMonotoneArray<ui64>::SaveForReadonlyAccess(out, *array.Lens_);
        }

        TBlob LoadFromBlob(const TBlob& blob) {
            return Lens.LoadFromBlob(Vals.LoadFromBlob(blob));
        }

        TVal operator[](size_t pos) const {
            return Get(pos);
        }

        TVal Get(size_t pos) const {
            return GetElement<TVal>(Vals, Lens, pos);
        }

    private:
        TReadonlyBitVector<ui64> Vals;
        TReadonlyEliasFanoMonotoneArray<ui64> Lens;
    };

}
