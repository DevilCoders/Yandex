#include "master_target_type.h"

#include "make_vector.h"
#include "master_list_manager.h"
#include "master_target_graph.h"
#include "messages.h"

#include <tools/clustermaster/common/cross_enumerator.h>
#include <tools/clustermaster/common/expand_params.h>
#include <tools/clustermaster/common/make_vector.h>
#include <tools/clustermaster/common/target_type_parameters_checksum.h>

#include <library/cpp/containers/sorted_vector/sorted_vector.h>

TMasterTargetType::TMasterTargetType(TMasterGraph* graph, const TConfigMessage*, const TString& name,
        const TVector<TVector<TString> >& paramss, TMasterListManager* listManager)
    : TTargetTypeBase<TMasterGraphTypes>(graph, name, paramss)

{
    TParamListManager::TListReference hostListReference = graph->GetParamListManager().GetOrCreateList(NTargetTypePrivate::ExpandHosts(paramss.at(0), listManager));
    TParamListManager::TListById hosts = Graph->GetParamListManager().GetList(hostListReference);

    TVector<TVector<TString> > expandedParams = ExpandParams(hosts.GetNames(), paramss, listManager);

    Parameters.Reset(new TTargetTypeParameters(
            GetName(),
            &Graph->GetParamListManager(),
            paramss.size(),
            expandedParams));

    CheckState();

    // Computing localspace shifts for workers
    for (TTargetTypeParameters::TIterator task = Parameters->Iterator(); task.Next(); ) {
        TString worker = task.CurrentPath()[0];
        if (LocalspaceShifts.find(worker) == LocalspaceShifts.end()) {
            LocalspaceShifts[worker] = task.CurrentN().GetN();
        }
    }
}

/*
 * Here we assume that cluster set is similar for all hosts.
 * If it wasn't, it will never end up in hosts list.
 */
TMaybe<TParamListManager::TListReference> TMasterTargetType::IsScatteringFor(const TMasterTargetType* other) const {
    if (other->GetParamCount() == 1
            && GetHostListReference() == other->GetParameters().GetListReferenceAtLevel(2))
    {
        return GetHostListReference();
    } else {
        return TMaybe<TParamListManager::TListReference>();
    }
}

TMaybe<TParamListManager::TListReference> TMasterTargetType::IsGatheringFor(const TMasterTargetType* other) const {
    return other->IsScatteringFor(this);
}

bool TMasterTargetType::IsEqualTo(const TMasterTargetType* other) const {
    return Parameters->IsEqualTo(other->Parameters.Get());
}

const TIdForString& TMasterTargetType::GetHostList() const{
    return GetParameters().GetNameListAtLevel(TTargetTypeParameters::HOST_LEVEL_ID);
}


bool TMasterTargetType::HostListDifferentWith(const TMasterTargetType* other) const {
    return GetHostListReference() != other->GetHostListReference();
}

ui64 TMasterTargetType::ChecksumForWorker(const TString& worker) const {
    TTargetTypeParameters::TIterator it = Parameters->IteratorParamFixed(
            Parameters->GetLevelId(TTargetTypeParameters::HOST_LEVEL_ID),
            GetHostId(worker));
    return Checksum(it);
}

void TMasterTargetType::DumpStateExtra(TPrinter& printer) const {
    TPrinter next = printer.Next();
    Parameters->DumpState(next);
}

void TMasterTargetType::CheckState() const {
    //TPrinter printer;
    //DumpState(printer);

    TTargetTypeBase<TMasterGraphTypes>::CheckState();

    Parameters->CheckState();
}
