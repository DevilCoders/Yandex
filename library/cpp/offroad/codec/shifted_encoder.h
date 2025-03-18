#pragma once

#include <util/generic/array_ref.h>

namespace NOffroad {
    template <size_t shift, class Base>
    class TShiftedEncoder: public Base {
        using TBase = Base;

    public:
        template <class Range>
        void Write(size_t channel, const Range& chunk) {
            TBase::Write(channel + shift, chunk);
        }

        template <class Unsigned>
        void Write(size_t channel, const TArrayRef<const Unsigned>& chunk) {
            TBase::Write(channel + shift, chunk);
        }
    };

    template <size_t shift, class Encoder>
    TShiftedEncoder<shift, Encoder>* AsShiftedEncoder(Encoder* encoder) {
        return static_cast<TShiftedEncoder<shift, Encoder>*>(encoder);
    }

}
