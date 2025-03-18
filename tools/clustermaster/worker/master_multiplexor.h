#pragma once

#include "worker_workflow.h"

#include <util/generic/hash_set.h>
#include <util/stream/format.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

class TMasterMultiplexor {
public:
    TMasterMultiplexor()
    {
    }

    virtual ~TMasterMultiplexor()
    {
    }

    void AddMaster(TWorkerControlWorkflow& m) {
        TGuard<TMutex> guard(Mutex);

        if (Masters.find(&m) != Masters.end()) {
            LOG("Masters list already contains this thread");
            ythrow yexception() << "attempt to add duplicated thread into multiplexor";
        }

        Masters.insert(&m);

        DEBUGLOG("Added workflow " << Hex(reinterpret_cast<size_t>(&m)) << " to Masters list"); // added logging here while solving CLUSTERMASTER-59
    }

    void RemoveMaster(TWorkerControlWorkflow& m) {
        TGuard<TMutex> guard(Mutex);

        TMastersSet::iterator master = Masters.find(&m);

        if (master == Masters.end()) {
            DEBUGLOG("Masters list doesn't contain "  << Hex(reinterpret_cast<size_t>(&m)) <<
                    " workflow (this is ok - a workflow is added to the list only after a successful auth)");
        } else {
            Masters.erase(master);
            DEBUGLOG("Removed workflow " << Hex(reinterpret_cast<size_t>(&m)) << " from Masters list"); // added logging here while solving CLUSTERMASTER-59
        }
    }

    template <typename TMessage>
    void SendToAll(const TMessage& message) {
        TGuard<TMutex> guard(Mutex);

        for (TMastersSet::iterator i = Masters.begin(); i != Masters.end(); ++i) {
            (*i)->EnqueueMessage(message);
        }
    }

    TString GetMasterHttpAddr() noexcept {
        TGuard<TMutex> guard(Mutex);

        for (const auto master : Masters) {
            if (master->Initialized() && master->IsPrimary()) {
                LastMasterHttpAddr = master->GetMasterHost() + ":" + ToString(master->GetMasterHttpPort());
                break;
            }
        }

        return LastMasterHttpAddr;
    }

    int GetNumMasters() const {
        return Masters.size();
    }

    int GetNumPrimaryMasters() const {
        TGuard<TMutex> guard(Mutex);

        int n = 0;
        for (TMastersSet::const_iterator i = Masters.begin(); i != Masters.end(); ++i)
            if ((*i)->IsPrimary())
                n++;

        return n;
    }

protected:
    using TMastersSet = THashSet<TWorkerControlWorkflow*>;

protected:
    TMutex Mutex;

    TMastersSet Masters;
    TString LastMasterHttpAddr;
};
