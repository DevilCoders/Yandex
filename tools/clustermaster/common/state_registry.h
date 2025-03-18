#pragma once

// XXX: Add to generic/alhorithm.h

#include "target.h"

#include <algorithm>

class TStateCounter {
public:
    TStateCounter(int count = 0, const TTaskState& state = TS_UNKNOWN): States(TStateRegistry::GetStatesCount(), 0), TotalStates(count) {
        if (count > 0) {
            States[TStateRegistry::find(state)] += count;
        }
    }

    inline void AddStates(int count, const TTaskState& state = TS_UNKNOWN) {
        States[TStateRegistry::find(state)] += count;
        TotalStates += count;
    }

    inline void ResetStates(int count, const TTaskState& state = TS_UNKNOWN) {
        std::fill(States.begin(), States.end(), 0);

        States[TStateRegistry::find(state)] = count;
        TotalStates = count;
    }

    inline int GetCount(const TTaskState& state) const {
        return States[TStateRegistry::find(state)];
    }

    inline int GetTotalCount() const {
        return TotalStates;
    }

    inline bool HasState(const TTaskState& state) const {
        return States[TStateRegistry::find(state)] > 0;
    }

    inline bool IsCompletely(const TTaskState& state) const {
        return States[TStateRegistry::find(state)] == TotalStates;
    }

    inline void ChangeState(const TTaskState& fromstate, const TTaskState& tostate, int count = 1) {
        size_t from = TStateRegistry::find(fromstate);
        Y_ASSERT(States[from] >= count);

        if (!fromstate.Equal(tostate)) {
            size_t to = TStateRegistry::find(tostate);
            States[from] -= count;
            States[to] += count;
        }
    }

    inline TStateCounter operator+(const TStateCounter& other) const {
        Y_ASSERT(States.size() == other.States.size());

        TStateCounter sum;
        sum.TotalStates = TotalStates + other.TotalStates;
        sum.States.reserve(States.size());

        for (unsigned int i = 0; i < States.size(); ++i)
            sum.States.push_back(States[i] + other.States[i]);

        return sum;
    }

    inline TStateCounter& operator+=(const TStateCounter& other) {
        Y_ASSERT(States.size() == other.States.size());

        TotalStates += other.TotalStates;

        for (unsigned int i = 0; i < States.size(); ++i)
            States[i] += other.States[i];

        return *this;
    }

protected:
    typedef TVector<int> StateArray;

protected:
    StateArray States;
    int TotalStates;
};
