#pragma once

#include "precomputed_task_ids_hyperspace.h"
#include "worker_target_graph_types.h"

#include <tools/clustermaster/common/depend.h>

class TWorkerDepend: public TDependBase<TWorkerGraphTypes> {
private:
    bool Local; // Means that this depend's source has no crossnode depends and it (source) is here
            // on this worker. For such depends we can calculate 'pokes' locally (see TryReadySomeTasks)

    TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<TWorkerGraphTypes> > PrecomputedTasksIdsMaybeHyperspace;
    TMaybe<TParamMappings> JoinParamMappingsHyperspace;

public:
    TWorkerDepend(
            TWorkerTarget* source, TWorkerTarget* target, bool invert,
            const TTargetParsed::TDepend&);

    void DumpState(TPrinter& out) const override;

    bool IsLocal() const { return Local; }

    TPrecomputedTaskIdsMaybe<TWorkerGraphTypes>* GetPrecomputedTaskIdsMaybeHyperspace() const;
    void SetPrecomputedTaskIdsMaybeHyperspace(TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<TWorkerGraphTypes> > taskIdsMaybeHyperspace);

    const TMaybe<TParamMappings>& GetJoinParamMappingsHyperspace() const;
};
