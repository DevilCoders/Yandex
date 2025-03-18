#pragma once

#include <util/generic/vector.h>
#include <util/stream/null.h>

#include <library/cpp/offroad/streams/bit_output.h>

#include "flat_type.h"
#include "flat_item.h"
#include "flat_common.h"

namespace NOffroad {
    template <class Key, class Data, class KeyVectorizer, class DataVectorizer, EFlatType flatWriterType = DefaultFlatType>
    class TFlatWriter {
    public:
        using TKey = Key;
        using TData = Data;

    private:
        using TItem = TFlatItem<KeyVectorizer::TupleSize, DataVectorizer::TupleSize>;
        using TKeyArray = typename TItem::TKeyArray;
        using TDataArray = typename TItem::TDataArray;

    public:
        TFlatWriter() {
            Reset(nullptr);
        }

        TFlatWriter(IOutputStream* output) {
            Reset(output);
        }

        ~TFlatWriter() {
            Finish();
        }

        void Reset(IOutputStream* output) {
            Items_.clear();
            Output_ = output ? output : &Cnull;
            IsFinished_ = false;
            IsFirstKey_ = true;
        }

        void WriteSeekPoint() {
            Y_ASSERT(!IsFinished_);
            if (flatWriterType == UniqueFlatType && !IsFirstKey_) {
                IsFirstKey_ = true;
                WriteLast();
            }
        }

        void Write(const TKey& key, const TData& data) {
            Y_ASSERT(!IsFinished_);
            if (flatWriterType == UniqueFlatType) {
                KeyVectorizer::Scatter(key, TempKey_);
                if (!IsFirstKey_ && (LastKey_ != TempKey_)) {
                    WriteLast();
                }
                LastKey_.swap(TempKey_);
                LastData_ = data;
                IsFirstKey_ = false;
            } else {
                TItem& item = Items_.emplace_back();
                KeyVectorizer::Scatter(key, item.Key());
                DataVectorizer::Scatter(data, item.Data());
            }
        }

        size_t Size() const {
            return Items_.size() + ((flatWriterType == UniqueFlatType) ? ((IsFirstKey_ || IsFinished_) ? 0 : 1) : 0);
        }

        void Finish() {
            if (IsFinished_)
                return;
            IsFinished_ = true;
            if (flatWriterType == UniqueFlatType && !IsFirstKey_) {
                WriteLast();
            }

            /* Just don't write out anything if we don't have data. */
            if (Items_.empty())
                return;

            TItem bits;
            bits.fill(0);
            for (const TItem& item : Items_)
                for (size_t i = 0; i < item.size(); i++)
                    bits[i] |= item[i];

            ui32 totalBits = 0;
            for (size_t i = 0; i < bits.size(); i++) {
                bits[i] = Bitness(bits[i]);
                totalBits += bits[i];
            }

            /* We don't support large keys for now. Might add support in the future
             * if needed. */
            ui32 keyBits = 0;
            for (size_t i = 0; i < bits.KeySize(); i++)
                keyBits += bits.Key(i);
            if (keyBits > NPrivate::MaxFastTupleBits)
                NPrivate::ThrowFlatSearcherKeyTooLongException();

            /* And we store bitness in ui8, so need to check for it too. */
            ui32 dataBits = totalBits - keyBits;
            if (dataBits > NPrivate::MaxSlowTupleBits)
                NPrivate::ThrowFlatSearcherDataTooLongException();

            /* Unlikely scenario, but still.
             *
             * If the total number of bits is too small then we'll have troubles
             * determining the number of records stored. So we throw in more bits
             * into data. */
            if (totalBits < 8) {
                bits.back() += 8 - totalBits;
                totalBits = 8;
            }

            TBitOutput output(Output_);
            for (size_t i = 0; i < bits.size(); i++)
                output.Write(bits[i], 6);

            for (const TItem& item : Items_)
                WriteTuple(&output, item.Key(), bits.Key());
            for (const TItem& item : Items_)
                WriteTuple(&output, item.Data(), bits.Data());

            Items_.clear();
        }

        bool IsFinished() const {
            return IsFinished_;
        }

    private:
        void WriteLast() {
            TItem& item = Items_.emplace_back();
            item.Key().swap(LastKey_);
            DataVectorizer::Scatter(LastData_, item.Data());
        }

        static ui32 Bitness(ui32 value) {
            return value == 0 ? 0 : MostSignificantBit(value) + 1;
        }

        template <class Tuple, class Bits>
        static void WriteTuple(TBitOutput* output, const Tuple& tuple, const Bits& bits) {
            /* We write out everything in reverse order so that we can take
             * advantage of it on little endian architectures. */
            for (size_t i = tuple.size(); i > 0; i--)
                output->Write(tuple[i - 1], bits[i - 1]);
        }

        TVector<TItem> Items_;
        IOutputStream* Output_ = nullptr;
        bool IsFinished_ = false;
        bool IsFirstKey_ = true;
        TKeyArray TempKey_;
        TKeyArray LastKey_;
        TData LastData_;
    };

}
