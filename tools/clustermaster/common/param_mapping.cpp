#include "param_mapping.h"

#include "id_for_string.h"
#include "precomputed_task_ids.h"
#include "target_type_parameters.h"

#include <util/generic/yexception.h>
#include <util/stream/str.h>

TParamMapping::TParamMapping(const TLevelId& myLevelId, const TLevelId& depLevelId)
    : MyLevelId(myLevelId)
    , DepLevelId(depLevelId)
{
    TParamListManager::TListReference myListReference = myLevelId.GetTargetType()->GetListReferenceAtLevel(myLevelId);
    TParamListManager::TListReference depListReference = depLevelId.GetTargetType()->GetListReferenceAtLevel(depLevelId);
    if (myListReference != depListReference) {
        ythrow TWithBackTrace<yexception>() << "list references mismatch";
    }
}


TString TParamMappings::ToString() const {
    if (Mappings.empty()) {
        return "*";
    } else {
        TStringStream ss;
        ss << "[";
        for (TVector<TParamMapping>::const_iterator m = Mappings.begin(); m != Mappings.end(); ++m) {
            if (m != Mappings.begin()) {
                ss << ",";
            }
            ss << (m->GetDepLevelId().GetValue() - 1) << "->" << (m->GetMyLevelId().GetValue() - 1);
        }
        ss << "]";
        return ss.Str();
    }
}

bool TParamMappings::Has00() const {
    for (TVector<TParamMapping>::const_iterator mapping = Mappings.begin();
            mapping != Mappings.end(); ++mapping)
    {
        if (mapping->GetMyLevelId().GetValue() == TTargetTypeParameters::HOST_LEVEL_ID
                && mapping->GetDepLevelId().GetValue() == TTargetTypeParameters::HOST_LEVEL_ID)
        {
            return true;
        }
    }
    return false;
}


TJoinCounter::TJoinCounter(
        const TTargetTypeParameters* myParameters,
        const TTargetTypeParameters* depParameters,
        const TParamMappings& mappings)
    : MyParameters(myParameters)
    , DepParameters(depParameters)
    , ParamListManager(myParameters->GetParamListManager())
    , Mappings(mappings)
{
    if (ParamListManager != depParameters->GetParamListManager())
        ythrow yexception() << "list managers mismatch";
}

bool TJoinCounter::SingleStep() {
    if (!State) {
        TVector<TIdForString::TIdSafe> ids;

        for (TVector<TParamMapping>::const_iterator mapping = Mappings.GetMappings().begin();
                mapping != Mappings.GetMappings().end(); ++mapping)
        {
            const TIdForString& list = MyParameters->GetNameListAtLevel(mapping->GetMyLevelId());
            const TIdForString& depList = DepParameters->GetNameListAtLevel(mapping->GetDepLevelId());
            if (&list != &depList) {
                ythrow TWithBackTrace<yexception>() << "lists mismatch";
            }
            if (list.Size() == 0) {
                ythrow TWithBackTrace<yexception>() << "cannot build projection from empty";
            }
            ids.push_back(list.GetFirstId());
        }
        State.Reset(new TState);
        DoSwap(ids, State->Ids);
        return true;
    }

    for (ssize_t i = Mappings.Size() - 1; i >= 0; --i) {
        const TIdForString& list = MyParameters->GetNameListAtLevel(Mappings.GetMappings().at(i).GetMyLevelId());
        TIdForString::TIdSafe& id = State->Ids.at(i);
        if (id != list.GetLastId()) {
            ++(State->Ids.at(i));
            return true;
        }
        id = list.GetFirstId();
    }

    return false;
}

bool TJoinCounter::Initialized() const {
    return !!State;
}

TJoinEnumerator::TJoinEnumerator(
        const TTargetTypeParameters* myParameters,
        const TTargetTypeParameters* depParameters,
        const TParamMappings& mappings)
    : Counter(myParameters, depParameters, mappings)
{}

bool TJoinEnumerator::Next() {
    return Counter.SingleStep();
}

TTargetTypeParameters::TProjection TJoinEnumerator::GetProjectionImpl(bool my) const {
    TVector<TMaybe<TIdForString::TIdSafe> > r;
    r.resize((my ? Counter.MyParameters : Counter.DepParameters)->GetDepth());
    for (size_t i = 0; i < Counter.Mappings.Size(); ++i) {
        const TParamMapping& mapping = Counter.Mappings.GetMappings().at(i);
        r.at((my ? mapping.GetMyLevelId() : mapping.GetDepLevelId()).GetValue() - 1) = Counter.State->Ids.at(i);
    }
    return TTargetTypeParameters::TProjection(r);
}

TTargetTypeParameters::TProjection TJoinEnumerator::GetMyProjection() const {
    return GetProjectionImpl(true);
}

TTargetTypeParameters::TProjection TJoinEnumerator::GetDepProjection() const {
    return GetProjectionImpl(false);
}

