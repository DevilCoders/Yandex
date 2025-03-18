#pragma once

#include "master_target_graph_types.h"
#include "target_type.h"
#include "target_type_parameters.h"

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

class TMasterListManager;
struct TConfigMessage;

class TMasterTargetType : public TTargetTypeBase<TMasterGraphTypes> {
public:
    // host id within target type, from 0 to Hosts.size()
    typedef TIdForString::TId THostId;
    typedef TIdForString::TIdSafe THostIdSafe;

    typedef THashMap<TString, THostId> THostIdByHost;

    // 0..(task count)
    typedef TIdForString::TId     TParamValueIdUnsafe;
    typedef TIdForString::TIdSafe TParamValueId;

    typedef TTargetTypeParameters::TId TTaskId;

    typedef TVector<TParamValueIdUnsafe> TParamValueIdUnsafes;

    typedef ui32 TWorkerLocalTaskId;

    typedef TMap<TString, size_t> TLocalspaceShifts;

    TMasterTargetType(TMasterGraph* graph, const TConfigMessage* msg, const TString& name,
            const TVector<TVector<TString> >& paramss, TMasterListManager* listManager);

    TMaybe<TParamListManager::TListReference> IsScatteringFor(const TMasterTargetType* other) const;
    TMaybe<TParamListManager::TListReference> IsGatheringFor(const TMasterTargetType* other) const;
    bool IsEqualTo(const TMasterTargetType* other) const;

    TTaskId GetTaskIdByWorkerIdAndLocalTaskN(THostIdSafe hostId, ui32 localTaskN) const {
        TTargetTypeParameters::TIterator it = Parameters->IteratorParamFixed(TTargetTypeParameters::HOST_LEVEL_ID, hostId);
        if (!it.Next())
            ythrow TWithBackTrace<yexception>() << "no first task " << localTaskN;
        it.Skip(localTaskN);
        if (it->at(0) != hostId) {
            ythrow TWithBackTrace<yexception>() << "wrong local id " << localTaskN;
        }
        return it.CurrentN();
    }

    TParamValueId GetTaskIdSafe(TParamValueIdUnsafe taskId) const {
        if (GetParameters().GetDepth() != 2) {
            ythrow yexception() << "can generate safe task id only for types with single parameter";
        }
        return GetParameters()
                .GetNameListAtLevel(TTargetTypeParameters::FIRST_PARAM_LEVEL_ID)
                .GetSafeId(taskId);
    }


    size_t GetTaskCountByWorker(const TString& worker) const {
        return GetTaskCountByWorker(GetHostIdSafe(worker));
    }

    size_t GetTaskCountByWorker(THostIdSafe hostId) const {
        return Parameters->GetCountFirstLevelFixed(hostId);
    }

    size_t GetTaskCount() const {
        return Parameters->GetCount();
    }

    /** very strange method - try avoid using it */
    size_t GetParamCountAtSecondLevel() const {
        if (Parameters->GetDepth() == 1) {
            return 1;
        } else if (Parameters->GetDepth() == 2) {
            return Parameters->GetParamCountAtLevel(2);
        } else {
            ythrow yexception() << "invalid depth: " << Parameters->GetDepth();
        }
    }

    const TVector<TString>& GetTaskNames() const {
        if (Parameters->GetDepth() != 2) {
            ythrow yexception() << "expecting 2 parameters (incl host), got " << Parameters->GetDepth();
        } else {
            return Parameters->GetParamNamesAtLevel(2);
        }
    }

    TParamValueIdUnsafe GetTaskIdByTaskName(const TString& taskName) const {
        if (Parameters->GetDepth() != 2) {
            ythrow yexception() << "expecting 2 parameters (incl host), got " << Parameters->GetDepth();
        } else {
            return Parameters->GetNameListAtLevel(2).GetIdByName(taskName).Id;
        }
    }

    const TTargetTypeParameters& GetParameters() const {
        return *Parameters;
    }

    const TIdForString& GetHostList() const;

    const TParamListManager::TListReference& GetHostListReference() const {
        return Parameters->GetListReferenceAtLevel(TTargetTypeParameters::HOST_LEVEL_ID);
    }

    const TVector<TString>& GetHosts() const {
        return GetHostList().GetNames();
    }

    size_t GetHostCount() const {
        return GetHostList().Size();
    }

    const TString& GetSingleHost() const {
        return GetHostList().GetSingleName();
    }

    const TString& GetHostById(THostId hostId) const {
        return GetHostList().GetNameById(hostId);
    }

    THostId GetHostId(const TString& host) const {
        return GetHostList().GetIdByName(host).Id;
    }

    THostIdSafe GetHostIdSafe(const TString& host) const {
        return GetHostIdSafe(GetHostList().GetIdByName(host).Id);
    }

    THostIdSafe GetHostIdSafe(THostId hostId) const {
        return GetHostList().GetSafeId(hostId);
    }

    bool HasHostname(const TString& host) {
        return GetHostList().HasName(host);
    }

    bool HostListDifferentWith(const TMasterTargetType* other) const;

    ui64 ChecksumForWorker(const TString& worker) const;

    void DumpStateExtra(TPrinter& printer) const override;

    void CheckState() const override;

    const TLocalspaceShifts& GetLocalspaceShifts() const { return LocalspaceShifts; };

private:
    THolder<TTargetTypeParameters> Parameters;

    TLocalspaceShifts LocalspaceShifts;
};
