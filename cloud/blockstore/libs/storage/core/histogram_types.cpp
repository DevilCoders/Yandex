#include "histogram_types.h"

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

template <size_t BucketsCount>
const TVector<TString> GenerateNames(const std::array<ui64, BucketsCount>& limits)
{
    TVector<TString> names;
    for (ui32 i = 0; i < limits.size(); ++i) {
        if (i != limits.size()-1) {
            names.emplace_back(ToString(limits[i]));
        } else {
            names.emplace_back("Inf");
        }
    }
    return names;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

const TVector<TString> TRequestTimeBuckets::GetNames()
{
    static const TVector<TString> names = GenerateNames(Limits);
    return names;
}

////////////////////////////////////////////////////////////////////////////////

const TVector<TString> TQueueSizeBuckets::GetNames()
{
    static const TVector<TString> names = GenerateNames(Limits);
    return names;
}

}   // namespace NCloud::NBlockStore::NStorage

