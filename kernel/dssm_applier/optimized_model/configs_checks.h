#pragma once

#include <kernel/dssm_applier/optimized_model/protos/bundle_config.pb.h>


namespace NOptimizedModel {

    void CheckBundleConfig(const NBuildModels::NProto::TModelsBundleConfig& config);

} // NOptimizedModel
