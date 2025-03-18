#pragma once

#include <util/stream/output.h>

#include <library/cpp/offroad/offset/tuple_sub_offset.h>
#include <library/cpp/offroad/flat/flat_writer.h>

namespace NOffroad {
    template <class Vectorizer, class Base>
    class TSubWriter: public Base {
    public:
        using THit = typename Base::THit;
        using TPosition = TTupleSubOffset;

        TSubWriter() {
        }

        template <class... Args>
        TSubWriter(IOutputStream* subOutput, Args&&... args)
            : Base(std::forward<Args>(args)...)
            , SubBase_(subOutput)
        {
        }

        ~TSubWriter() {
            Finish();
        }

        template <class... Args>
        void Reset(IOutputStream* subOutput, Args&&... args) {
            Base::Reset(std::forward<Args>(args)...);
            SubBase_.Reset(subOutput);
        }

        void WriteHit(const THit& data) {
            Base::WriteHit(data);

            TDataOffset position = Base::Position();
            if (position.Index() == 0) {
                SubBase_.Write(data, position.Offset());
            }
        }

        TTupleSubOffset Position() const {
            return TTupleSubOffset(Base::Position(), SubBase_.Size());
        }

        void WriteSeekPoint() {
            Base::WriteSeekPoint();
            SubBase_.WriteSeekPoint();
        }

        void Finish() {
            if (Base::IsFinished())
                return;

            SubBase_.Finish();
            Base::Finish();
        }

    private:
        TFlatWriter<THit, ui64, Vectorizer, TUi64Vectorizer, UniqueFlatType> SubBase_;
    };

}
