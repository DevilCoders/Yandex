#pragma once

// a set of independent machines to be checked against
// needed to section the set to limit number of states

#include "machine.h"
#include <util/generic/vector.h>

namespace NHtmlDetect {
    class TContext {
    public:
        TContext(size_t id)
            : Machine(id)
        {
        }
        void PushState() {
            States.push_back(Machine.GetState());
        }
        void PopState() {
            Machine.SetState(States.back());
            States.pop_back();
        }
        void PushText(const char* text, size_t len) {
            Machine.Push(text, len);
        }
        TMachine& GetMachine() {
            return Machine;
        }

    private:
        typedef TVector<int> TStates;
        TStates States;
        TMachine Machine;
    };

    class TMachineSet {
    public:
        TMachineSet();
        void PushState();
        void PopState();
        void PushText(const char* text, size_t len);
        const TDetectResult& GetResult();

    private:
        void UpdateResult();

    private:
        typedef TVector<TContext> TMachines;
        typedef TMachines::iterator TIter;

        TMachines Machines;
        TDetectResult Result;
    };

}
