#pragma once

#include "condition.h"
#include "make_vector.h"
#include "param_mapping.h"
#include "param_mapping_detect.h"
#include "precomputed_task_ids.h"
#include "printer.h"
#include "target_graph_parser.h"
#include "target_type_parameters.h"

#include <util/generic/ptr.h>
#include <util/system/defaults.h>

enum EDependencyFlags {
    // UNUSED            = 0x0001,
    DF_BARRIER           = 0x0004,
    DF_GROUP_DEPEND      = 0x0008,
    DF_SEMAPHORE         = 0x0010,
    DF_NON_RECURSIVE     = 0x0020,
};

enum EDependMode {
    DM_UNDEFINED,
    DM_PROJECTION,
    DM_GROUPDEPEND
};

template <typename PTypes>
class TDependBase;

template <typename PTypes>
class TDependBase {
private:
    typedef typename PTypes::TTarget TTarget;

    TTarget* const Source;
    TTarget* const Target;
    const bool Invert; // support legacy field order

protected:
    ui32 Flags;
    const TCondition Condition;
    EDependMode Mode;

protected:
    TMaybe<TParamMappings> JoinParamMappings;

private:
    TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<PTypes> > PrecomputedTasksIdsMaybe;

public:
    TPrecomputedTaskIdsMaybe<PTypes>* GetPrecomputedTaskIdsMaybe() const {
        return PrecomputedTasksIdsMaybe.Get();
    }

    void SetPrecomputedTaskIdsMaybe(TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<PTypes> > taskIdsMaybe) {
        PrecomputedTasksIdsMaybe = taskIdsMaybe;
    }

    EDependMode GetMode() const {
        return Mode;
    }

public:
    TDependBase(TTarget* source, TTarget* target, bool invert, const TTargetParsed::TDepend& dependParsed)
        : Source(source)
        , Target(target)
        , Invert(invert)
        , Flags(dependParsed.Flags & (~DF_GROUP_DEPEND))
        , Condition(TCondition::Parse(dependParsed.Condition))
        , Mode(DM_UNDEFINED)
    {
#if 0 // TODO: restore
        if ((Flags & DF_CROSSNODE) && (Flags & DF_SEMAPHORE))
            ythrow yexception() << "semaphore dependency can't be crossnode"
                << " (" << GetRealSource()->GetName() << ": " << GetRealTarget()->GetName() << ")";
#endif
    }

    virtual ~TDependBase() {}

    ui32 GetFlags() const { return Flags; }

    // TODO: may be a source
    const TTarget* GetTarget() const {
        return Target;
    }

    TTarget* GetTarget() {
        return Target;
    }

    const TTarget* GetSource() const {
        return Source;
    }

    TTarget* GetSource() {
        return Source;
    }

    bool IsInvert() const {
        return Invert;
    }

    const TTarget* GetRealSource() const {
        return Invert ? Target : Source;
    }

    const TTarget* GetRealTarget() const {
        return Invert ? Source : Target;
    }

    TTarget* GetRealSource() {
        return Invert ? Target : Source;
    }

    TTarget* GetRealTarget() {
        return Invert ? Source : Target;
    }

    const TMaybe<TParamMappings>& GetJoinParamMappings() const {
        return JoinParamMappings;
    }

    const TCondition& GetCondition() const {
        return Condition;
    }

    virtual void DumpState(TPrinter& out) const {
        out.Println(GetRealTarget()->GetName());
    }

    template <typename PParamsByType>
    static TParamMappings MappingsFromParsedOrAutodetect(const TTargetParsed::TDepend& dependParsed,
            const typename PTypes::TTargetType& sourceType, const typename PTypes::TTargetType& targetType)
    {
        const TTargetTypeParameters& sourceParams = PParamsByType::GetParams(sourceType);
        const TTargetTypeParameters& targetParams = PParamsByType::GetParams(targetType);

        if (!!dependParsed.ParamMappings) {
            TVector<TParamMapping> paramMappings;

            for (TVector<TTargetParsed::TParamMapping>::const_iterator mappingParsed =
                    dependParsed.ParamMappings->begin();
                    mappingParsed != dependParsed.ParamMappings->end();
                    ++mappingParsed)
            {
                TLevelId myLevelId = sourceParams.GetLevelId(mappingParsed->MyLevelId + TTargetTypeParameters::HOST_LEVEL_ID);
                TLevelId depLevelId = targetParams.GetLevelId(mappingParsed->DepLevelId + TTargetTypeParameters::HOST_LEVEL_ID);
                paramMappings.push_back(TParamMapping(myLevelId, depLevelId));
            }

            return TParamMappings(paramMappings);
        } else {
            TParamMappings result = DetectMappings(targetParams, sourceParams);

            // DetectMappings works only if each list (for example list of walruses) is present in parameters once. If there are
            // (this is most simple case) two equal lists (walrus on walrus - or as it called WALRUSseghosts - type) it won't work.
            // So after DetectMappings we compare first lists (host lists) - if they are equal we return 0->0 mapping.
            if (result.GetMappings().empty()) {
                if (sourceParams.GetListReferenceAtLevel(TTargetTypeParameters::HOST_LEVEL_ID) ==
                        targetParams.GetListReferenceAtLevel(TTargetTypeParameters::HOST_LEVEL_ID))
                {
                    TLevelId myLevelId = sourceParams.GetLevelId(TTargetTypeParameters::HOST_LEVEL_ID);
                    TLevelId depLevelId = targetParams.GetLevelId(TTargetTypeParameters::HOST_LEVEL_ID);

                    result = TParamMappings(MakeVector(TParamMapping(myLevelId, depLevelId)));
                }
            }

            return result;
        }
    }
};

