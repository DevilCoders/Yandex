#pragma once

#include "async_jobs_pool.h"
#include "log.h"
#include "master_multiplexor.h"
#include "messages.h"
#include "worker_target.h"

#include <util/generic/algorithm.h>
#include <util/generic/noncopyable.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <algorithm>

class TGraphChangeWatcher: public TNonCopyable {
public:
    struct TChange: TSimpleRefCount<TChange> {
        const TWorkerTarget* const Target;
        const TTargetTypeParameters::TId nTask;
        const TTaskState OldState;
        const TTaskState NewState;
        const bool NeedsSingleStatus;

        TChange(const TWorkerTarget* target, TTargetTypeParameters::TId ntask, const TTaskState& oldState, const TTaskState& newState, bool needsSingleStatus) noexcept
            : Target(target)
            , nTask(ntask)
            , OldState(oldState)
            , NewState(newState)
            , NeedsSingleStatus(needsSingleStatus)
        {
        }
    };

    struct TChangeLess {
        bool operator()(const TIntrusivePtr<TChange>& first, const TIntrusivePtr<TChange>& second) const noexcept {
            return first->Target < second->Target;
        }
    };

public:
    typedef TVector< TIntrusivePtr<TChange> > TChanges;

private:
    TChanges Changes;
    static inline TAsyncJobsPool Pool;

public:
    void Register(const TWorkerTarget* target, TTargetTypeParameters::TId nTask, const TTaskState& oldState, const TTaskState& newState, bool needsSingleStatus) {
        Changes.push_back(new TChange(target, nTask, oldState, newState, needsSingleStatus));
    }

    const TChanges& GetChanges() const {
        return Changes;
    }

    enum {
        COMMIT_NETWORK = 1,
        COMMIT_DISK = 2,
        COMMIT_RESOURCES = 4,
        COMMIT_ALL = 0xFF,
    };

    TGraphChangeWatcher() noexcept
        : Changes()
    {
    }

    bool HasChanges() const noexcept {
        return !Changes.empty();
    }

    void Commit(unsigned flags = COMMIT_ALL) {
        if (Changes.empty())
            return;

        TGuard<TResourceManager, NCommunism::TGroupingOps> resourceActionsGroupingGuard(GetResourceManager());

        StableSort(Changes.begin(), Changes.end(), TChangeLess());

        TMasterMultiplexor& masters = *Singleton<TMasterMultiplexor>();
        bool thinStatusNeeded = false;

        DEBUGLOG("Commit: changes size "<<  Changes.size());
        auto diskJobs = Pool.Get(std::min<size_t>(Changes.size(), 64u));

        for (TChanges::const_iterator currentLowerBound = Changes.begin(), i = Changes.begin();; ++i) {
            if (i == Changes.end() || (*i)->Target != (*currentLowerBound)->Target) {
                if ((flags & COMMIT_NETWORK) && thinStatusNeeded) { // Network changes
                    masters.SendToAll(*(*currentLowerBound)->Target->CreateThinStatus());

                    thinStatusNeeded = false;
                }

                if (flags & COMMIT_DISK) { // Disk changes
                    const TWorkerTarget* target = (*currentLowerBound)->Target;
                    DEBUGLOG("Commit: adding disk job of " << target->GetName());
                    diskJobs.Add(THolder(new TSomeJob([target]() {
                        DEBUGLOG("Commit: start saving state of " << target->GetName());
                        target->SaveState();
                        DEBUGLOG("Commit: stop saving state of " << target->GetName());
                    })));
                }
                currentLowerBound = i;
            }

            if (i == Changes.end())
                break;

            if (flags & COMMIT_NETWORK) { // Network changes
                if ((*i)->NeedsSingleStatus)
                    masters.SendToAll(*(*i)->Target->CreateSingleStatus((*i)->nTask));
                else
                    thinStatusNeeded = true;
            }

            if (flags & COMMIT_RESOURCES) try { // Resources changes
                if ((*i)->NewState == TS_READY) { // Request
                    (*i)->Target->RequestResources((*i)->nTask);
                } else if ((*i)->NewState == TS_RUNNING) { // Claim
                    const TWorkerTaskStatus& task = (*i)->Target->GetTasks().At((*i)->nTask);
                    if (task.GetPid() == -1 || task.GetLostState()) { // Task don't use resources/forced run (task.GetPid() == -1) or need to reclaim resources (see CLUSTERMASTER-52) after worker restart (task.GetLostState())
                        (*i)->Target->ClaimResources((*i)->nTask);
                    }
                } else if ((*i)->OldState == TS_READY || (*i)->OldState == TS_RUNNING) // Disclaim
                    if ((*i)->NewState != TS_STOPPING) { // Don't disclaim if we are stopping process with SIGSTOP
                        (*i)->Target->DisclaimResources((*i)->nTask);
                    }
            } catch (const TWorkerTarget::TResourcesNotDefined& e) {
                ERRORLOG1(target, (*i)->Target->Name << " task " << (*i)->nTask.GetN() << ": " << e.what());
            }
        }
    }
};
