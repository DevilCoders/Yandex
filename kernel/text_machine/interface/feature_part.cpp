#include "feature_part.h"

using namespace NTextMachine;

template<>
void Out<TKValue>(IOutputStream& os, const TKValue& kValue)
{
    os << kValue.GetName() << kValue.Value;
}
template<>
void Out<TNormValue>(IOutputStream& os, const TNormValue& normValue)
{
    os << normValue.GetName() << normValue.Value;
}
template<>
void Out<TStreamIndexValue>(IOutputStream& os, const TStreamIndexValue& streamIndex)
{
    os << streamIndex.GetName() << streamIndex.Value;
}
template<>
void Out<TStreamSet>(IOutputStream& os, const TStreamSet& streamSet)
{
    if (streamSet.IsStream()) {
        os << streamSet.GetStream();
    } else {
        Y_ASSERT(streamSet.IsStreamSet());
        os << streamSet.GetStreamSet();
    }
}
