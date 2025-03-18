#pragma once

#include "sequence_caller.h"

template <class TException = yexception, bool doRethrow = true>
class TSafeCaller: public TSequenceCaller {
private:
    TMasterActionPoint RestorePoint;
    TString LastErrorMessage;

public:
    TSafeCaller()
        : TSequenceCaller("TSafeCaller")
        , RestorePoint(this, "restore")
    {
        Bind(this).template To<TString, &TSafeCaller::GetLastError>("output");
        Bind(this).template To<&TSafeCaller::Run>("run");
        AddPointDependency("run", "restore");
    }

    static TCalcModuleHolder BuildModule() {
        return new TSafeCaller;
    }

private:
    void Run() {
        try {
            for (TMasterActionPoints::iterator it = Sequence.begin(); it != Sequence.end(); it++) {
                it->DoAction();
            }
        } catch (const TException& e) {
            LastErrorMessage = e.what();
            RestorePoint.DoAction();
            if (doRethrow) {
                throw;
            }
        }
    }
    TString GetLastError() {
        return LastErrorMessage;
    }
};
