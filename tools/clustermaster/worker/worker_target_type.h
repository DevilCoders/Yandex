#pragma once

#include "target_type.h"
#include "worker_target_graph_types.h"

#include <tools/clustermaster/common/target_type_parameters.h>

class TWorkerListManager;
struct TConfigMessage;

class TWorkerTargetType : public TTargetTypeBase<TWorkerGraphTypes> {
public:
    typedef TIdForString::TIdSafe TTaskIdSafe;

    TWorkerTargetType(TWorkerGraph* graph, const TConfigMessage* msg, const TString& name,
            const TVector<TVector<TString> >& paramss, TWorkerListManager* listManager);

    bool IsEqualTo(const TWorkerTargetType* other) const;
    bool IsOnThisWorker() const;

    const TTargetTypeParameters& GetParameters() const;
    const TTargetTypeParameters& GetParametersHyperspace() const;

    ui32 GetParameterCountSafe() const {
        return GetParameters().GetCount();
    }

    void DumpStateExtra(TPrinter&) const override;

    TTargetTypeParameters::TId GetIdByScriptParams(const TVector<TString>& params);

    class TLocalspaceShift {
    public:
        TLocalspaceShift(int shift, int size)
            : Shift(shift)
            , Size(size)
        {
        }

        size_t GetShift() const {
            return Shift;
        }

        size_t GetSize() const {
            return Size;
        }

    private:
        size_t Shift;
        size_t Size;
    };

    const TMaybe<TLocalspaceShift>& GetLocalspaceShift() const {
        return LocalspaceShift;
    }

private:
    THolder<TTargetTypeParameters> Parameters;

    /**
     * ParametersHyperspace allows us to iterate through tasks on all workers (not only on this worker). We
     * need this to make calculation of subgraph for command distributed through workers. See TWorkerGraph::Command2(...).
     */
    THolder<TTargetTypeParameters> ParametersHyperspace;

    TMaybe<TLocalspaceShift> LocalspaceShift;

    bool ExistsOnThisWorker;
};
