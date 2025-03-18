#include "worker_depend.h"

#include "worker_target.h"

TWorkerDepend::TWorkerDepend(
        TWorkerTarget* source, TWorkerTarget* target, bool invert,
        const TTargetParsed::TDepend& dependParsed)
    : TDependBase<TWorkerGraphTypes>(source, target, invert, dependParsed)
    , Local(false)
{
    if (dependParsed.Flags & DF_GROUP_DEPEND) {
        Mode = DM_GROUPDEPEND;
    } else {
        Mode = DM_PROJECTION;
    }

    JoinParamMappingsHyperspace = MappingsFromParsedOrAutodetect<TParamsByTypeHyperspace>(dependParsed, *GetRealSource()->Type, *GetRealTarget()->Type);

    if (GetRealSource()->HasCrossnodeDepends || !GetRealSource()->Type->IsOnThisWorker()) {
        Local = false;
    } else {
        Local = true;

        JoinParamMappings = MappingsFromParsedOrAutodetect<TParamsByTypeOrdinary>(dependParsed, *GetRealSource()->Type, *GetRealTarget()->Type);
    }
}

void TWorkerDepend::DumpState(TPrinter& out) const {
    TDependBase<TWorkerGraphTypes>::DumpState(out);
    TPrinter l1 = out.Next();

    if (Mode == DM_GROUPDEPEND) {
        l1.Println("group depend");
    } else if (!Local) {
        l1.Println("not local");
    } else if (Mode == DM_PROJECTION) {
        l1.Println("projection: " + GetJoinParamMappings()->ToString());
    } else {
        l1.Println("unknown mode");
    }
}

TPrecomputedTaskIdsMaybe<TWorkerGraphTypes>* TWorkerDepend::GetPrecomputedTaskIdsMaybeHyperspace() const {
    return PrecomputedTasksIdsMaybeHyperspace.Get();
}

void TWorkerDepend::SetPrecomputedTaskIdsMaybeHyperspace(TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<TWorkerGraphTypes> > taskIdsMaybeHyperspace) {
    PrecomputedTasksIdsMaybeHyperspace = taskIdsMaybeHyperspace;
}

const TMaybe<TParamMappings>& TWorkerDepend::GetJoinParamMappingsHyperspace() const {
    return JoinParamMappingsHyperspace;
}

