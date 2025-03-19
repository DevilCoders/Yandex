#pragma once

#include <util/stream/output.h>
#include <util/ysaveload.h>

#pragma pack(1)
template<class TInd, class TFactor>
struct Y_PACKED TTIndFactor
{
    TInd    Ind;
    TFactor Factor;

    TTIndFactor(TInd ind = 0, TFactor factor = TFactor(0))
        : Ind(ind)
        , Factor(factor)
    {}
};
#pragma pack()

template<class TInd, class TFactor>
class TSerializer<TTIndFactor<TInd, TFactor>>:
    public TSerializerTakingIntoAccountThePodType<TTIndFactor<TInd, TFactor>, true> {};

typedef TTIndFactor<ui8,float> TIndFactor;

template<>
inline void Out<TIndFactor>(IOutputStream& o, const TIndFactor& t)
{
    o << '[' << ui64(t.Ind) << ',' << t.Factor << ']';
}
