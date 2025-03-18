#pragma once


#include <util/generic/ptr.h>

#include "robot_set.h"
#include "service_param_holder.h"


namespace NAntiRobot {


class TAmnestyFlags {
public:
    TAtomic ForAll = 0;
    TServiceParamHolder<TAtomic> ByService;

public:
    void SetRobots(TAtomicSharedPtr<TRobotSet> robots) {
        Robots = std::move(robots);
    }

    void SetAmnesty(EHostType service, bool value) {
        if (value) {
            Robots->Clear(service);
        }
    }

private:
    TAtomicSharedPtr<TRobotSet> Robots;
};


}
