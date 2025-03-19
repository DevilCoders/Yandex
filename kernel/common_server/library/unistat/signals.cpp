#include "signals.h"

TNamedSignalCustom::TNamedSignalCustom(const TString& baseName, const EAggregationType aggrInHost, const TString& suffix)
    : TBase(baseName, aggrInHost, suffix)
{

}

TNamedSignalSimple::TNamedSignalSimple(const TString& baseName)
    : TBase(baseName, false)
{

}

TNamedSignalHistogram::TNamedSignalHistogram(const TString& baseName, const TVector<double>& intervals)
    : TBase(baseName, intervals) {

}
