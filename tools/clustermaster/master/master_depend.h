#pragma once

#include "master_target_graph_types.h"

#include <tools/clustermaster/common/depend.h>
#include <tools/clustermaster/common/param_list_manager.h>
#include <tools/clustermaster/common/param_mapping.h>

#include <util/generic/maybe.h>

class TMasterDepend: public TDependBase<TMasterGraphTypes> {
private:
    bool Crossnode;

public:
    TMasterDepend(TMasterTarget* source, TMasterTarget* target, bool invert, const TTargetParsed::TDepend&);

    bool IsCrossnode() const { return Crossnode; }

    const TMaybe<TParamMappings>& GetJoinParamMappings() const {
        return JoinParamMappings;
    }

    void DumpState(TPrinter& out) const override;
};
