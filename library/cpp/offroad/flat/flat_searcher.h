#pragma once

#include <array>

#include <util/generic/xrange.h>
#include <util/generic/array_ref.h>
#include <util/generic/algorithm.h>
#include <util/memory/blob.h>

#include <library/cpp/offroad/utility/masks.h>

#include "flat_common.h"

namespace NOffroad {
    /**
     * Random access interface for reading whatever was written with `TFlatWriter`.
     */
    template <class Key, class Data, class KeyVectorizer, class DataVectorizer>
    class TFlatSearcher {
    public:
        enum {
            TupleSize = KeyVectorizer::TupleSize + DataVectorizer::TupleSize
        };

        using TKey = Key;
        using TData = Data;

        TFlatSearcher() {
        }

        TFlatSearcher(const TArrayRef<const char>& source) {
            Reset(source);
        }

        TFlatSearcher(const TBlob& blob) {
            Reset(blob);
        }

        void Reset() {
            Reset(TArrayRef<const char>());
        }

        void Reset(const TArrayRef<const char>& source) {
            Reset(TBlob::NoCopy(source.data(), source.size()));
        }

        void Reset(const TBlob& blob) {
            ResetInternal(blob);
        }

        size_t Size() const {
            return Size_;
        }

        TKey ReadKey(size_t index) const {
            return KeyAt(index);
        }

        TData ReadData(size_t index) const {
            return DataAt(index);
        }

        size_t LowerBound(const TKey& prefix) const {
            return LowerBound(prefix, 0, Size_);
        }

        size_t LowerBound(const TKey& prefix, size_t from, size_t to) const {
            Y_ASSERT(from <= to && to <= Size());
            if (Size_ == 0) {
                return 0;
            }
            return LowerBoundInternal(KeyToFastKey(prefix), from, to);
        }

    private:
        enum {
            KeyOffset_ = 6 * TupleSize
        };

        void ResetInternal(const TBlob& source) {
            Source_ = source;

            /* Empty source is valid, see writer. */
            if (source.Empty()) {
                Size_ = 0; /* No need to clear other fields here. */
                return;
            }

            InitBitSizes(source, std::make_index_sequence<KeyVectorizer::TupleSize>(), std::make_index_sequence<DataVectorizer::TupleSize>());
            if (!FastKey_)
                NPrivate::ThrowFlatSearcherKeyTooLongException();
        }

        template <size_t... ikey, size_t... idata>
        Y_FORCE_INLINE void InitBitSizes(const TBlob& header, const std::index_sequence<ikey...>&, const std::index_sequence<idata...>&) {
            ui32 keyBits = 0;
            ui32 dataBits = 0;

            constexpr size_t lkey = sizeof...(ikey) - 1;
            constexpr size_t ldata = sizeof...(idata) - 1;

            size_t bits = 0;
            auto dummy = {
                (
                    KeyFieldOffsets_[lkey - ikey] = keyBits,
                    bits = NPrivate::SelectBitsFromFlatHeader<lkey - ikey>(header),
                    KeyFieldMasks_[lkey - ikey] = ScalarMask(bits),
                    keyBits += bits)...,
                (
                    DataFieldOffsets_[ldata - idata] = dataBits,
                    bits = NPrivate::SelectBitsFromFlatHeader<lkey + 1 + ldata - idata>(header),
                    DataFieldMasks_[ldata - idata] = ScalarMask(bits),
                    dataBits += bits)...};
            Y_UNUSED(dummy);

            KeyBits_ = keyBits;
            DataBits_ = dataBits;
            FastKey_ = keyBits <= NPrivate::MaxFastTupleBits;
            FastData_ = dataBits <= NPrivate::MaxFastTupleBits;
            FastKeyMask_ = ScalarMask(keyBits);

            ui32 totalBits = keyBits + dataBits;
            Size_ = (Source_.Size() * 8 - KeyOffset_) / totalBits;
            DataOffset_ = KeyOffset_ + keyBits * Size_;
        }

        size_t LowerBoundInternal(ui64 prefix, size_t from, size_t to) const {
            auto cmp = [&](size_t l, ui64 r) {
                return FastKeyAt(l) < r;
            };

            auto range = xrange(from, to);
            return *::LowerBound(range.begin(), range.end(), prefix, cmp);
        }

        Y_FORCE_INLINE ui64 KeyToFastKey(const TKey& key) const {
            std::array<ui32, KeyVectorizer::TupleSize> tmp;
            KeyVectorizer::Scatter(key, tmp);

            ui64 result = 0;
            for (size_t i = 0; i < KeyVectorizer::TupleSize; i++)
                result += static_cast<ui64>(tmp[i]) << KeyFieldOffsets_[i];

            return result;
        }

        Y_FORCE_INLINE TKey KeyAt(size_t index) const {
            Y_ASSERT(FastKey_);

            return LoadTupleFast<TKey, KeyVectorizer>(Source_, KeyPtr(index), KeyFieldMasks_, KeyFieldOffsets_);
        }

        Y_FORCE_INLINE ui64 FastKeyAt(size_t index) const {
            return NPrivate::LoadBits(Source_, KeyPtr(index), FastKeyMask_);
        }

        Y_FORCE_INLINE TData DataAt(size_t index) const {
            if (Y_LIKELY(FastData_)) {
                return LoadTupleFast<TData, DataVectorizer>(Source_, DataPtr(index), DataFieldMasks_, DataFieldOffsets_);
            } else {
                return LoadTupleSlow<TData, DataVectorizer>(Source_, DataPtr(index), DataFieldMasks_, DataFieldOffsets_);
            }
        }

        Y_FORCE_INLINE ui64 KeyPtr(size_t index) const {
            Y_ASSERT(index < Size_);

            return KeyOffset_ + KeyBits_ * index;
        }

        Y_FORCE_INLINE ui64 DataPtr(size_t index) const {
            Y_ASSERT(index < Size_);

            return DataOffset_ + DataBits_ * index;
        }

        template <class T, class Vectorizer, class Masks, class Offsets>
        Y_FORCE_INLINE static T LoadTupleFast(const TBlob& blob, ui64 offset, const Masks& masks, const Offsets& offsets) {
            std::array<ui32, Vectorizer::TupleSize> tmp;

            ui64 bits = NPrivate::LoadBits(blob, offset);
            for (size_t i = 0; i < Vectorizer::TupleSize; i++)
                tmp[i] = (bits >> offsets[i]) & masks[i];

            T result;
            Vectorizer::Gather(tmp, &result);
            return result;
        }

        template <class T, class Vectorizer, class Masks, class Offsets>
        Y_FORCE_INLINE static T LoadTupleSlow(const TBlob& blob, ui64 offset, const Masks& masks, const Offsets& offsets) {
            std::array<ui32, Vectorizer::TupleSize> tmp;

            for (size_t i = 0; i < Vectorizer::TupleSize; i++)
                tmp[i] = NPrivate::LoadBits(blob, offset + offsets[i], masks[i]);

            T result;
            Vectorizer::Gather(tmp, &result);
            return result;
        }

    private:
        size_t Size_ = 0;
        ui64 DataOffset_ = 0;
        ui64 FastKeyMask_ = 0;
        ui8 KeyBits_ = 0;
        ui8 DataBits_ = 0;
        bool FastKey_ = false;
        bool FastData_ = false;
        std::array<ui8, KeyVectorizer::TupleSize> KeyFieldOffsets_;
        std::array<ui8, DataVectorizer::TupleSize> DataFieldOffsets_;
        std::array<ui64, KeyVectorizer::TupleSize> KeyFieldMasks_;
        std::array<ui64, DataVectorizer::TupleSize> DataFieldMasks_;
        TBlob Source_;
    };

}
