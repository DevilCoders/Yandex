#pragma once

#include "copy_points.h"
#include "slave_action_points.h"

#include <util/system/type_name.h>

class TMasterActionPoint: public TMasterCopyPoint<TActionFunction> {
private:
    typedef TMasterCopyPoint<TActionFunction> TBase;

protected:
    /// @throws TIncompatibleAccessPoints
    void CheckCompatibility(const TSlaveAccessPoint& slaveAccessPoint) const override {
        if (typeid(slaveAccessPoint) != typeid(TSlaveActionPoint)) {
            ythrow TIncompatibleAccessPoints() << "TMasterActionPoint: Connecting not compatible access points: " << TypeName(slaveAccessPoint) << " != " << TypeName<TSlaveActionPoint>();
        }
    }

public:
    TMasterActionPoint()
        : TBase(&DefaultAction)
    {
    }
    TMasterActionPoint(TSimpleModule* module, const TString& names)
        : TBase(module, &DefaultAction, names)
    {
    }

    void DoAction() {
        try {
            Val(GetOwner());
        } catch (const TUninitModuleException&) {
            TBase::ThrowUninitException("TMasterActionPoint");
        }
    }

private:
    static void DefaultAction(void*) {
        ythrow TUninitModuleException();
    }
};
