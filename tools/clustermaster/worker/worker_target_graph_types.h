#pragma once

class TWorkerGraph;
class TWorkerTarget;
class TWorkerTargetType;
class TWorkerListManager;
class TWorkerDepend;
class TWorkerTaskStatus;
struct TWorkerPrecomputedTaskIdsContainer;

struct TWorkerGraphTypes {
    typedef TWorkerGraph TGraph;
    typedef TWorkerTarget TTarget;
    typedef TWorkerTargetType TTargetType;
    typedef TWorkerListManager TListManager;
    typedef TWorkerDepend TDepend;
    typedef TWorkerTaskStatus TSpecificTaskStatus;
    typedef TWorkerPrecomputedTaskIdsContainer TPrecomputedTaskIdsContainer;
};

