#pragma once

#include <array>

#include <library/cpp/offroad/offset/offset_data.h>

#include <util/generic/ylimits.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/null.h>

namespace NOffroad {
    template <class KeyData, class Serializer>
    class TFatWriter {
    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TKeyData = KeyData;
        using TSerializer = Serializer;

        TFatWriter() {
            Reset(nullptr, nullptr);
        }

        TFatWriter(IOutputStream* fat, IOutputStream* fatsub) {
            Reset(fat, fatsub);
        }

        ~TFatWriter() {
            Finish();
        }

        void Reset(IOutputStream* fat, IOutputStream* fatsub) {
            Fat_ = fat ? fat : &Cnull;
            FatSub_ = fatsub ? fatsub : &Cnull;
            Offset_ = 0;
            SubIndex_ = 0;
            WriteKey(TKey(), TKeyData()); /* Dummy record to make search-previous simpler. */
        }

        void WriteKey(const TKeyRef& key, const TKeyData& data) {
            Y_VERIFY(key.size() <= Max<ui16>()); /* We store length as ui16, thus this limit. */

            std::array<ui8, Serializer::MaxSize> tmp;
            size_t dataSize = Serializer_.Serialize(data, tmp.data());
            ui16 keySize = key.size();

            IOutputStream::TPart parts[] = {
                {&keySize, sizeof(keySize)},
                {key.data(), key.size()},
                {tmp.data(), dataSize},
            };

            ++SubIndex_;
            WriteFatSubOffset();

            Offset_ += parts[0].len + parts[1].len + parts[2].len;
            if (Offset_ > Max<ui32>())
                ythrow yexception() << "Fat is expected to fit in 4Gb.";

            Fat_->Write(parts, 3);
        }

        void Finish() {
            if (IsFinished_)
                return;

            IsFinished_ = true;
        }

        ui32 Size() const {
            return SubIndex_;
        }

    private:
        void WriteFatSubOffset() {
            ui32 offset = Offset_;
            FatSub_->Write(&offset, sizeof(offset));
        }

    private:
        TSerializer Serializer_;
        IOutputStream* Fat_;
        IOutputStream* FatSub_;
        ui64 Offset_ = 0;
        ui32 SubIndex_ = 0;
        bool IsFinished_ = false;
    };

    template <class KeyData, class Serializer>
    class TFatOffsetDataWriter: public TFatWriter<TOffsetData<KeyData>, TOffsetDataSerializer<Serializer>> {
        using TBase = TFatWriter<TOffsetData<KeyData>, TOffsetDataSerializer<Serializer>>;

    public:
        using TBase::TBase;
    };

}
