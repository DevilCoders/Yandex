#pragma once

#include "worker_target_type.h"

#include <tools/clustermaster/common/precomputed_task_ids.h>

class TWorkerTargetType;
struct TWorkerGraphTypes;

/**
 * One of two PParamsByType template argument 'implementations'.
 *
 * We need PParamsByType because gather subgraph algorithm and all code for precomputing dependency task ids
 * should work with both ordinary and worker specific 'hyperspace' tasks (see TWorkerTargetType::HyperspaceParameters).
 *
 * See also TParamsByTypeOrdinary 'implementation'.
 */
struct TParamsByTypeHyperspace {
    static const TTargetTypeParameters& GetParams(const TWorkerTargetType& type) {
        return type.GetParametersHyperspace();
    }

    template <typename PTypes>
    static TPrecomputedTaskIdsMaybe<PTypes>& GetPrecomputedTaskIdsMaybe(const typename PTypes::TDepend& dep) {
        return *dep.GetPrecomputedTaskIdsMaybeHyperspace();
    }
};

typedef TPrecomputedTaskIdsInitializer<TWorkerGraphTypes, TParamsByTypeHyperspace> TPrecomputedTaskIdsInitializerHyperspace;
typedef TPrecomputedTaskIdsContainer<TWorkerGraphTypes, TParamsByTypeHyperspace> TPrecomputedTaskIdsContainerHyperspace;
