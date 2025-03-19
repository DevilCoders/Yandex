#pragma once

#include <kernel/factors_info/factors_info.h>
#include <kernel/factor_storage/factor_storage.h>

// This header file is auto-generated from factors_gen.in
#include <kernel/itditp_user_history_factors_info/factors_gen.h>

namespace NItdItpUserHistory {

const size_t N_FACTOR_COUNT = NItdItpUserHistory::FI_FACTOR_COUNT;

const IFactorsInfo* GetItdItpUserHistoryFactorsInfo();

}  // namespace NItdItpUserHistory
