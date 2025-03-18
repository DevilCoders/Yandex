#include "state_registry.h"

#include "target.h"

static const TStateRegistry::TStateInfo States[] = {
    { TS_IDLE,       "idle",      "idle",      "Idle",      "i" },
    { TS_PENDING,    "pending",   "pending",   "Pending",   "p" },
    { TS_READY,      "ready",     "ready",     "Ready",     "e" },
    { TS_RUNNING,    "running",   "running",   "Running",   "r" },
    { TS_STOPPING,   "stopping",  "stopping",  "Stopping",  "L" },
    { TS_STOPPED,    "stopped",   "stopped",   "Stopped",   "S" },
    { TS_CONTINUING, "continuing","continuing","Continuing","C" },
    { TS_SUSPENDED,  "suspended", "suspended", "Suspended", "U" },
    { TS_SUCCESS,    "success",   "success",   "Success",   "s" },
    { TS_SKIPPED,    "skipped",   "skipped",   "Skipped",   "k" },
    { TS_FAILED,     "failed",    "failed",    "Failed",    "f" },
    { TTaskState(TS_FAILED, SIGKILL), "killed",   "killed",    "Killed",   "fk" },
    { TTaskState(TS_FAILED, SIGABRT), "aborted",  "aborted",   "Aborted",  "fa" },
    { TTaskState(TS_FAILED, SIGSEGV), "violated", "violated",  "Violated", "fv" },
    { TS_DEPFAILED, "depfailed", "depfailed", "Depfailed", "d" },
    { TS_CANCELING, "canceling", "canceling", "Canceling", "l" },
    { TS_CANCELED,  "canceled",  "canceled",  "Canceled",  "c" },
    { TS_UNKNOWN,   "unknown",   "unknown",   "Unknown",   "u" },
};

static const size_t NumStates = Y_ARRAY_SIZE(States);


typedef TStateRegistry::TStateInfo TStateInfo;

size_t TStateRegistry::GetStatesCount() {
    return NumStates;
}

TStateRegistry::const_iterator TStateRegistry::begin() {
    return States;
}

TStateRegistry::const_iterator TStateRegistry::end() {
    return States + NumStates;
}

size_t TStateRegistry::find(const TTaskState& state)
{
    for (size_t i = 0; i < NumStates; ++i) {
        const TStateInfo* si = &States[i];
        if (state.Equal(si->State)) {
            return i;
        }
    }

    // If process was killed by a non-listed signal, consider it 'failed'
    if (state == TS_FAILED) {
        return find(TS_FAILED);
    }

    ythrow yexception() << "bad target state";
}

const TStateInfo* TStateRegistry::findByState(const TTaskState& state)
{
    size_t i = TStateRegistry::find(state);
    return &States[i];
}

const TStateInfo* TStateRegistry::findBySmallName(const TString &name)
{
    for (size_t i = 0; i < NumStates; ++i) {
        const TStateInfo* si = &States[i];
        if (name.equal(si->SmallName)) {
            return si;
        }
    }
    ythrow yexception() << "bad target state name";
}
