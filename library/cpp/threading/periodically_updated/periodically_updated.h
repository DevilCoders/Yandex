#pragma once

#include <util/datetime/base.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

class TPeriodicallyUpdated {
public:
    TPeriodicallyUpdated(TDuration reloadDuration, TDuration errorReloadDuration,
                         const TString& threadName = {});
    virtual ~TPeriodicallyUpdated();
    //! return true - success initialization, false - timeout
    bool WaitForFirstUpdate(unsigned timeoutSeconds);

    void Stop(); // should be called at least in destructor to stop updates
protected:
    virtual bool UpdateImpl() = 0; // true is success, false if error
    void Start();
    void WaitForStop(TDuration sleepDuration);

    void SetReloadDuration(TDuration reloadDuration);

private:
    TDuration mReloadDuration, mErrorReloadDuration, mCheckPeriod;

    bool mStopping;
    bool mStarted;
    bool mUpdated;
    TMutex mMutex;
    TCondVar mStopCond;
    TCondVar mUpdateCond;

    TInstant mNextReloadTime;
    TThread mTimerThread;

    static void* UpdateFunc(void* param) {
        ((TPeriodicallyUpdated*)param)->Update();
        return nullptr;
    }

    bool SetStopping();
    bool GetStopping();

    void Update();
};
