#pragma once

#include "config.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/thread/pool.h>

namespace NRemorphCompiler {

class TCompileTask: public IObjectInQueue, public TSimpleRefCount<TCompileTask> {
public:
    typedef TIntrusivePtr<TCompileTask> TPtr;
    typedef TVector<TPtr> TPtrs;

    struct INotifier {
        virtual void Notify() = 0;
    };

private:
    const TUnitConfig& Unit;
    IOutputStream* Log;
    bool Status;
    TString Error;

    INotifier* Notifier;

public:
    TCompileTask(const TUnitConfig& unit, IOutputStream* log = nullptr);

    void Process(void* ThreadSpecificResource) override;

    void SetNotifier(INotifier* notifier);
    void ResetError();
    void SetError(const TString& error);

    inline const TUnitConfig& GetUnit() const {
        return Unit;
    }

    inline bool GetStatus() const {
        return Status;
    }

    inline const TString& GetError() const {
        return Error;
    }
};

} // NRemorphCompiler
