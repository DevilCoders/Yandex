#pragma once

// This header file is auto-generated from factors_gen.in
#include <kernel/images_nn_over_dssm_doc_features/factors/factors_gen.h>

#include <kernel/factors_info/factors_info.h>
#include <kernel/factor_storage/factor_storage.h>

class IFactorsInfo;

namespace NImagesNnOverDssmDocFeatures {

    const size_t N_FACTOR_COUNT = NImagesNnOverDssmDocFeatures::FI_FACTOR_COUNT;

    const IFactorsInfo* GetImagesFactorsInfo();

}  // namespace NImagesNnOverDssmDocFeatures
