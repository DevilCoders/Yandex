#include "worker_target_type.h"

#include "messages.h"
#include "worker.h"
#include "worker_list_manager.h"
#include "worker_target_graph.h"

#include <tools/clustermaster/common/cross_enumerator.h>
#include <tools/clustermaster/common/expand_params.h>
#include <tools/clustermaster/common/make_vector.h>
#include <tools/clustermaster/common/target_type_parameters_checksum.h>

#include <util/generic/bt_exception.h>
#include <util/system/defaults.h>

TWorkerTargetType::TWorkerTargetType(
        TWorkerGraph* graph, const TConfigMessage* msg, const TString& name,
        const TVector<TVector<TString> >& paramss, TWorkerListManager* listManager)
    : TTargetTypeBase<TWorkerGraphTypes>(graph, name, paramss)
    , ExistsOnThisWorker(false)
{
    bool skipChecksum = true;
    TMaybe<ui64> expectedChecksum;
    for (google::protobuf::RepeatedPtrField<TConfigMessage::TThisWorkerTargetType>::const_iterator myType
            = msg->GetThisWorkerTargetTypes().begin();
            myType != msg->GetThisWorkerTargetTypes().end(); ++myType)
    {
        if (myType->GetName() == name) {
            ExistsOnThisWorker = true;
            if (myType->HasChecksum()) {
                expectedChecksum = myType->GetChecksum();
                skipChecksum = false;
            } else {
                skipChecksum = true;
            }
            break;
        }
    }

    if (!skipChecksum) {
        Y_UNUSED(*expectedChecksum);
    }

    TVector<TString> firsts;
    if (ExistsOnThisWorker) {
        firsts.push_back(Graph->GetWorkerNameCheckDefined());
    }

    TVector<TVector<TString> > expandedParams = ExpandParams(firsts, paramss, listManager);

    Parameters.Reset(new TTargetTypeParameters(
            GetName(),
            &Graph->GetParamListManager(),
            paramss.size(),
            expandedParams));

    // Evaluating hyperspace parameters

    TParamListManager::TListReference hostListReference = Graph->GetParamListManager().GetOrCreateList(NTargetTypePrivate::ExpandHosts(paramss.at(0), listManager));
    TParamListManager::TListById hosts = Graph->GetParamListManager().GetList(hostListReference);
    TVector<TVector<TString> > expandedHyperspaceParams = ExpandParams(hosts.GetNames(), paramss, listManager);

    ParametersHyperspace.Reset(new TTargetTypeParameters(
            GetName(),
            &Graph->GetParamListManager(),
            paramss.size(),
            expandedHyperspaceParams,
            true));

    CheckState();

    // Computing the localspace shift

    if (IsOnThisWorker()) {
        size_t localspaceShift = 0;
        bool shiftWasFound = false;
        for (TTargetTypeParameters::TIterator task = ParametersHyperspace->Iterator(); task.Next(); ) {
            TString worker = task.CurrentPath()[0];
            if (worker == Graph->GetWorkerNameCheckDefined()) {
                localspaceShift = task.CurrentN().GetN();
                shiftWasFound = true;
                break;
            }
        }
        Y_VERIFY(shiftWasFound);
        size_t localspaceSize = expandedParams.size();
        LocalspaceShift = TLocalspaceShift(localspaceShift, localspaceSize);
    }

    if (!ExistsOnThisWorker) {
        return;
    }

    ui64 actualChecksum = Checksum(Parameters->Iterator());
    if (!skipChecksum && actualChecksum != *expectedChecksum) {
        ythrow yexception() << "checksums mismatch for target type " << name
                << ", expected " << *expectedChecksum << ", actual " << actualChecksum;
    }
}

bool TWorkerTargetType::IsEqualTo(const TWorkerTargetType* other) const {
    if (!Parameters || !other->Parameters) {
        // weird legacy behavior
        return !Parameters == !other->Parameters;
    } else {
        // TODO: must compare parameters on all hosts, not just on this
        return Parameters->IsEqualTo(other->Parameters.Get());
    }
}

bool TWorkerTargetType::IsOnThisWorker() const {
    return ExistsOnThisWorker;
}

TTargetTypeParameters::TId TWorkerTargetType::GetIdByScriptParams(const TVector<TString>& params) {
    TVector<TString> fullPath = Concat(MakeVector(Graph->GetWorkerNameCheckDefined()), params);
    return GetParameters().GetNForPath(fullPath);
}


const TTargetTypeParameters& TWorkerTargetType::GetParameters() const {
    return *Parameters;
}

const TTargetTypeParameters& TWorkerTargetType::GetParametersHyperspace() const {
    return *ParametersHyperspace;
}

void TWorkerTargetType::DumpStateExtra(TPrinter& out) const {
    if (!IsOnThisWorker()) {
        out.Println("not on this worker");
        return;
    }
    Parameters->DumpState(out);
}
