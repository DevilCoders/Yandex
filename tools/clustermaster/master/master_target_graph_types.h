#pragma once

class TMasterGraph;
class TMasterTarget;
class TMasterTargetType;
class TMasterListManager;
class TMasterDepend;
struct TMasterTaskStatus;
struct TMasterPrecomputedTaskIdsContainer;

struct TMasterGraphTypes {
    typedef TMasterGraph TGraph;
    typedef TMasterTarget TTarget;
    typedef TMasterTargetType TTargetType;
    typedef TMasterListManager TListManager;
    typedef TMasterDepend TDepend;
    typedef TMasterTaskStatus TSpecificTaskStatus;
    typedef TMasterPrecomputedTaskIdsContainer TPrecomputedTaskIdsContainer;
};

