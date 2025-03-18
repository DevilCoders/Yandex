#pragma once

#include "blob_reader.h"

#include <library/cpp/containers/bitseq/bitvector.h>
#include <library/cpp/containers/bitseq/readonly_bitvector.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>
#include <util/ysaveload.h>

namespace NSuccinctArrays {
    class TReadonlyIntVector;

    class TIntVector: public TBitVector<ui64> {
        using TTraits = TBitVector<ui64>::TTraits;
        friend class TReadonlyIntVector;

        ui8 Width_;
        TWord FullMask_;

    public:
        TIntVector(ui64 size = 0, ui8 width = 0)
            : TBitVector<TWord>(size * width)
            , Width_(width)
            , FullMask_(TTraits::ElemMask(Width_))
        {
        }

        template <typename I>
        TIntVector(I begin, I end, ui8 width)
            : TBitVector<TWord>((end - begin) * width)
            , Width_(width)
            , FullMask_(TTraits::ElemMask(Width_))
        {
            for (size_t i = 0; begin != end; ++i, ++begin)
                Set(i, *begin);
        }

        template <typename I>
        void Assign(I begin, I end, ui8 width) {
            Width_ = width;
            Resize((end - begin) * Width_);
            for (size_t i = 0; begin != end; ++i, ++begin) {
                Set(i, *begin);
            }
        }

        template <typename I>
        void Assign(I begin, I end) {
            Assign(begin, end, sizeof(I::value_type) * CHAR_BIT);
        }

        ~TIntVector() override {
        }

        void Set(ui64 idx, ui64 val) {
            TBitVector<TWord>::Set(idx * Width_, val, Width_, FullMask_);
        }

        void Append(ui64 val) {
            TBitVector<TWord>::Append(val, Width_, FullMask_);
        }

        void push_back(ui64 val) {
            Append(val);
        }

        void PushBack(ui64 val) {
            Append(val);
        }

        ui64 Get(ui64 idx) const {
            return TBitVector<TWord>::Get(idx * Width_, Width_, FullMask_);
        }

        ui64 operator[](ui64 idx) const {
            return Get(idx);
        }

        ui8 Width() const {
            return Width_;
        }

        void Save(IOutputStream* out) const {
            ::Save(out, Width_);
            ::Save(out, FullMask_);
            TBitVector<TWord>::Save(out);
        }

        void Load(IInputStream* inp) {
            ::Load(inp, Width_);
            ::Load(inp, FullMask_);
            TBitVector<TWord>::Load(inp);
        }

        void Swap(TIntVector& other) {
            DoSwap(Width_, other.Width_);
            DoSwap(FullMask_, other.FullMask_);
            TBitVector<TWord>::Swap(other);
        }
    };

    class TReadonlyIntVector: public TReadonlyBitVector<ui64> {
    public:
        TReadonlyIntVector()
            : Width_()
            , FullMask_()
        {
        }

        explicit TReadonlyIntVector(const TIntVector& vector)
            : TReadonlyBitVector<ui64>(vector)
            , Width_(vector.Width_)
            , FullMask_(vector.FullMask_)
        {
        }

        ui64 operator[](size_t pos) const {
            return Get(pos);
        }

        ui64 Get(ui64 idx) const {
            return TReadonlyBitVector<TWord>::Get(idx * Width_, Width_, FullMask_);
        }

        ui8 Width() const {
            return Width_;
        }

        ui64 Size() const {
            return TReadonlyBitVector<ui64>::Size() / Width_;
        }

        static void SaveForReadonlyAccess(IOutputStream* out, const TIntVector& iv) {
            ::Save(out, static_cast<ui64>(iv.Width_));
            ::Save(out, iv.FullMask_);
            TReadonlyBitVector<TWord>::SaveForReadonlyAccess(out, iv);
        }

        TBlob LoadFromBlob(const TBlob& blob) override {
            TBlobReader reader(blob);

            ui64 width{};
            reader.ReadInteger(&width);
            Y_ENSURE(
                width <= std::numeric_limits<ui8>::max(),
                "width is too large; make sure you used SaveAligned() when saving data");
            Width_ = static_cast<ui8>(width);

            reader.ReadInteger(&FullMask_);
            return TReadonlyBitVector<ui64>::LoadFromBlob(reader.Tail());
        }

    private:
        ui8 Width_;
        TWord FullMask_;
    };

}
