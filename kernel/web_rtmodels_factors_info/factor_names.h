#pragma once

#include <kernel/factors_info/factors_info.h>
#include <kernel/factor_storage/factor_storage.h>

// This header file is auto-generated from factors_gen.in
#include <kernel/web_rtmodels_factors_info/factors_gen.h>

namespace NWebRtModels {

const size_t N_FACTOR_COUNT = NWebRtModels::FI_FACTOR_COUNT;

const IFactorsInfo* GetWebRtModelsFactorsInfo();

}  // namespace NWebMetaItdItp
