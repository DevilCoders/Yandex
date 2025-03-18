#pragma once

#include <tools/clustermaster/common/target_type_parameters.h>

#include <util/generic/vector.h>

class TGraphChangeWatcher;
class TWorkerTarget;

class TSemaphore {
public:
    TSemaphore(TWorkerTarget* target);

    void SetLimit(unsigned int limit);

    void AddTarget(TWorkerTarget* target);

    void Update();
    bool TryRun(const TTargetTypeParameters::TId& nTask);

    void TryReadySomeTasks(TGraphChangeWatcher& watcher);

    bool IsLast(TWorkerTarget* target) const;

protected:
    TVector<TWorkerTarget*> Targets;
    TVector<bool> BusyMask;
    unsigned int Used;
    unsigned int Width;
    unsigned int Limit;
};
