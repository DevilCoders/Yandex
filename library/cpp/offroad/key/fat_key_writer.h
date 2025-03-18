#pragma once

#include <utility>

#include <library/cpp/offroad/fat/fat_writer.h>
#include <library/cpp/offroad/offset/offset_data.h>
#include <library/cpp/offroad/offset/data_offset.h>
#include <library/cpp/offroad/offset/tuple_sub_offset.h>

namespace NOffroad {
    template <class FatWriter, class Base>
    class TFatKeyWriter: public Base {
        using TBase = Base;
        using TFatWriter = FatWriter;

        static_assert(std::is_same<typename TBase::TKeyRef, typename TFatWriter::TKeyRef>::value, "Base class is expected to use same key as FatWriter");
        static_assert(std::is_same<TOffsetData<typename TBase::TKeyData>, typename TFatWriter::TKeyData>::value, "Base class is expected to use same data as FatWriter");

    public:
        using TKey = typename TBase::TKey;
        using TKeyRef = typename TBase::TKeyRef;
        using TKeyData = typename TBase::TKeyData;

        TFatKeyWriter() {
        }

        template <class... Args>
        TFatKeyWriter(IOutputStream* fatOutput, IOutputStream* fatSubOutput, Args&&... args)
            : TBase(std::forward<Args>(args)...)
        {
            ResetInternal(fatOutput, fatSubOutput);
        }

        template <class... Args>
        void Reset(IOutputStream* fatOutput, IOutputStream* fatSubOutput, Args&&... args) {
            TBase::Reset(std::forward<Args>(args)...);
            ResetInternal(fatOutput, fatSubOutput);
        }

        ~TFatKeyWriter() {
            Finish();
        }

        void WriteKey(const TKeyRef& key, const TKeyData& data) {
            TBase::WriteKey(key, data);

            TDataOffset position = TBase::Position();
            if (position.Index() == 0)
                FatWriter_.WriteKey(key, TOffsetData<TKeyData>(position.Offset(), data));
        }

        void Finish() {
            if (TBase::IsFinished())
                return;

            if (!TBase::LastKey().empty()) {
                FatWriter_.WriteKey(TBase::LastKey(), TOffsetData<TKeyData>(TBase::Position().Offset(), TKeyData()));
            }
            FatWriter_.Finish();
            TBase::Finish();
        }

        TTupleSubOffset Position() const {
            return TTupleSubOffset(Base::Position(), FatWriter_.Size());
        }

    private:
        void ResetInternal(IOutputStream* fatOutput, IOutputStream* fatSubOutput) {
            FatWriter_.Reset(fatOutput, fatSubOutput);
        }

    private:
        TFatWriter FatWriter_;
    };

}
