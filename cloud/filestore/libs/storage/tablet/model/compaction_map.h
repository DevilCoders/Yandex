#pragma once

#include "public.h"

#include <cloud/filestore/libs/storage/tablet/model/range.h>

#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TCompactionStats
{
    ui32 BlobsCount;
    ui32 DeletionsCount;
};

////////////////////////////////////////////////////////////////////////////////

struct TCompactionRangeInfo
{
    ui32 RangeId;
    TCompactionStats Stats;
};

////////////////////////////////////////////////////////////////////////////////

class TCompactionMap
{
private:
    struct TImpl;
    std::unique_ptr<TImpl> Impl;

public:
    TCompactionMap();
    ~TCompactionMap();

    void Update(ui32 rangeId, ui32 blobsCount, ui32 deletionsCount);

    TCompactionStats Get(ui32 rangeId) const;

    TCompactionCounter GetTopCompactionScore() const;
    TCompactionCounter GetTopCleanupScore() const;

    TVector<TCompactionRangeInfo> GetTopRangesByCompactionScore(ui32 topSize) const;
    TVector<TCompactionRangeInfo> GetTopRangesByCleanupScore(ui32 topSize) const;
};

}   // namespace NCloud::NFileStore::NStorage
