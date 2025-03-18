#include "master_depend.h"

#include "master_target.h"

TMasterDepend::TMasterDepend(
        TMasterTarget* source, TMasterTarget* target, bool invert,
        const TTargetParsed::TDepend& dependParsed)
    : TDependBase<TMasterGraphTypes>(source, target, invert, dependParsed)
{
    if (dependParsed.Flags & DF_GROUP_DEPEND) {
        Mode = DM_GROUPDEPEND;
        Crossnode = true;
    } else {
        Mode = DM_PROJECTION;

        JoinParamMappings = MappingsFromParsedOrAutodetect<TParamsByTypeOrdinary>(dependParsed,
                *GetRealSource()->Type, *GetRealTarget()->Type);

        // TODO: handle a case of single host
        // when mapping is specified explicitly but does not specify 0->0,
        // that is implied because there's only host
        Crossnode = !JoinParamMappings->Has00();
    }
}

void TMasterDepend::DumpState(TPrinter& out) const {
    TDependBase<TMasterGraphTypes>::DumpState(out);
    TPrinter l1 = out.Next();

    if (Mode == DM_PROJECTION) {
        l1.Println("projection: " + GetJoinParamMappings()->ToString());
    } else if (Mode == DM_GROUPDEPEND) {
        l1.Println("group depend");
    } else {
        l1.Println("unknown mode");
    }
}
