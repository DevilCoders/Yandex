#pragma once

#include "action_points.h"
#include "simple_module.h"

#include <util/generic/vector.h>
#include <util/string/printf.h>

class TSequenceCaller: public TSimpleModule {
protected:
    typedef TVector<TMasterActionPoint> TMasterActionPoints;
    TMasterActionPoints Sequence;

    TSequenceCaller(const TString& moduleName)
        : TSimpleModule(moduleName)
    {
        Bind(this).To<IAccessPoint*, &TSequenceCaller::AddPoint>("actionpoint_input");
    }

public:
    TSequenceCaller()
        : TSimpleModule("TSequenceCaller")
    {
        Bind(this).To<IAccessPoint*, &TSequenceCaller::AddPoint>("actionpoint_input");
        Bind(this).To<&TSequenceCaller::Run>("run");
    }

    static TCalcModuleHolder BuildModule() {
        return new TSequenceCaller;
    }

private:
    void AddPoint(IAccessPoint* newPoint) {
        TMasterActionPoint newPointMaster;
        newPointMaster.Connect(*newPoint);
        TMasterActionPoint* start = (Sequence.size()) ? &Sequence[0] : nullptr;
        Sequence.push_back(newPointMaster);
        size_t pointId = Sequence.size() - 1;
        TString pointName = ToString(pointId);
        if (start == &Sequence[0]) {
            AddAccessPoint(pointName, &Sequence.back());
        } else {
            for (size_t i = 0; i < Sequence.size(); i++) {
                ReplaceAccessPoint(ToString(i), &Sequence[i]);
            }
        }
        if (pointId) {
            AddPointDependency(ToString(pointId - 1), pointName);
        } else {
            AddPointDependency("run", pointName);
        }
    }
    void Run() {
        for (TMasterActionPoints::iterator it = Sequence.begin(); it != Sequence.end(); it++) {
            it->DoAction();
        }
    }
};
