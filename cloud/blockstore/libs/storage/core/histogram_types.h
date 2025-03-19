#pragma once

#include "public.h"

#include <util/generic/vector.h>
#include <util/string/cast.h>

#include <array>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TRequestTimeBuckets
{
    static constexpr size_t BUCKETS_COUNT = 15;

    static constexpr std::array<ui64, BUCKETS_COUNT> Limits = {{
        1000, 2000, 5000,
        10000, 20000, 50000,
        100000, 200000, 500000,
        1000000, 2000000, 5000000,
        10000000, 35000000, Max<ui64>()
    }};

    static const TVector<TString> GetNames();
};

////////////////////////////////////////////////////////////////////////////////

struct TQueueSizeBuckets
{
    static constexpr size_t BUCKETS_COUNT = 15;

    static constexpr std::array<ui64, BUCKETS_COUNT> Limits = {{
        1, 2, 5,
        10, 20, 50,
        100, 200, 500,
        1000, 2000, 5000,
        10000, 35000, Max<ui64>()
    }};

    static const TVector<TString> GetNames();
};

}   // namespace NCloud::NBlockStore::NStorage

