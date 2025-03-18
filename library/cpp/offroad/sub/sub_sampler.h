#pragma once

#include <library/cpp/offroad/offset/tuple_sub_offset.h>
#include <library/cpp/offroad/flat/flat_writer.h>

namespace NOffroad {
    template <class Vectorizer, class Base>
    class TSubSampler: public Base {
    public:
        using THit = typename Base::THit;
        using TPosition = TTupleSubOffset;

        TSubSampler() {
        }

        template <class... Args>
        TSubSampler(Args&&... args)
            : Base(std::forward<Args>(args)...)
        {
        }

        template <class... Args>
        void Reset(Args&&... args) {
            Base::Reset(std::forward<Args>(args)...);
            SubBase_.Reset(nullptr);
        }

        void Write(const THit& data) {
            Base::Write(data);

            TDataOffset position = Base::Position();
            if (position.Index() == 0) {
                SubBase_.Write(data, position.Offset());
            }
        }

        TTupleSubOffset Position() {
            return TTupleSubOffset(Base::Position(), SubBase_.Size());
        }

    private:
        TFlatWriter<THit, ui64, Vectorizer, TUi64Vectorizer> SubBase_;
    };

}
