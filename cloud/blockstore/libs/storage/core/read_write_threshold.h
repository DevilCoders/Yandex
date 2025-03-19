#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/config.h>

#include <ydb/core/tablet/tablet_counters.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

TPerfThreshold GetThreshold(
    const TDiagnosticsConfig& config,
    bool isSsd);

bool CheckPercentileLimit(
    ui64 threshold,
    double percentile,
    const NKikimr::TTabletPercentileCounter& small,
    const NKikimr::TTabletPercentileCounter& large);

}   // namespace NCloud::NBlockStore::NStorage
