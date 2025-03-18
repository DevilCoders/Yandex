#pragma once

#include <util/generic/array_ref.h>

namespace NOffroad {
    template <size_t shift, class Base>
    class TShiftedDecoder: public Base {
        using TBase = Base;

    public:
        template <class Range>
        size_t Read(size_t channel, Range* chunk) {
            return TBase::Read(channel + shift, chunk);
        }

        template <class Unsigned>
        size_t Read(size_t channel, TArrayRef<Unsigned> chunk) {
            return TBase::Read(channel + shift, chunk);
        }
    };

    template <size_t shift, class Decoder>
    TShiftedDecoder<shift, Decoder>* AsShiftedDecoder(Decoder* decoder) {
        return static_cast<TShiftedDecoder<shift, Decoder>*>(decoder);
    }

}
