#include "periodically_updated.h"

TPeriodicallyUpdated::TPeriodicallyUpdated(TDuration reloadDuration, TDuration errorReloadDuration,
                                           const TString& threadName)
    : mReloadDuration(reloadDuration)
    , mErrorReloadDuration(errorReloadDuration)
    , mCheckPeriod(TDuration::Seconds(2))
    , mStopping(false)
    , mStarted(false)
    , mUpdated(false)
    , mNextReloadTime(TInstant::Now())
    , mTimerThread(TThread::TParams(UpdateFunc, this).SetName(threadName.size() ? threadName : "TPeriod")) {
}

void TPeriodicallyUpdated::Start() {
    mTimerThread.Start();
    with_lock (mMutex) {
        mStarted = true;
    }
}

bool TPeriodicallyUpdated::SetStopping() {
    with_lock (mMutex) {
        if (mStopping)
            return false;
        mStopping = true;
    }
    mStopCond.Signal();
    return true;
}

bool TPeriodicallyUpdated::GetStopping() {
    TGuard<TMutex> l(mMutex);
    return mStopping;
}

void TPeriodicallyUpdated::WaitForStop(TDuration sleepDuration) {
    bool ret = true;
    TGuard<TMutex> l(mMutex);
    while (!mStopping && ret) {
        ret = mStopCond.WaitT(mMutex, sleepDuration);
    }
}

bool TPeriodicallyUpdated::WaitForFirstUpdate(unsigned timeoutSeconds) {
    TGuard<TMutex> l(mMutex);
    Y_ENSURE(mStarted, "Start() should be called before WaitForFirstUpdate()\n");
    TInstant startTime = Now();
    TDuration timeSpent = TDuration::MicroSeconds(0);
    while (!mStopping && !mUpdated && timeSpent < TDuration::Seconds(timeoutSeconds)) {
        mUpdateCond.WaitT(mMutex, TDuration::MicroSeconds(1000));
        TInstant currentTime = Now();
        timeSpent = currentTime - startTime;
    }
    return mUpdated;
}

void TPeriodicallyUpdated::Update() {
    while (!GetStopping()) {
        if (TInstant::Now() > mNextReloadTime) {
            try {
                bool success = UpdateImpl();
                if (success)
                    mNextReloadTime = TInstant::Now() + mReloadDuration;
                else
                    mNextReloadTime = TInstant::Now() + mErrorReloadDuration;
            } catch (yexception &ex) {
                Cerr << "Exception in UpdateImpl" << Endl;
                Cerr << ex.what() << Endl;
                mNextReloadTime = TInstant::Now() + mErrorReloadDuration;
            }
            with_lock (mMutex) {
                if (!mUpdated) {
                    mUpdated = true;
                    mUpdateCond.BroadCast();
                }
            }
        }
        WaitForStop(mCheckPeriod);
    }
}

void TPeriodicallyUpdated::Stop() {
    if (SetStopping()) // TMutex::Acquire never throws, see ./system/mutex.h
        mTimerThread.Join(); //TThread::Join never throws (pg guarantee)
}

TPeriodicallyUpdated::~TPeriodicallyUpdated() {
    if (!GetStopping()) {
        try {
            Cerr << (void*)this << " " << "warning: Stop should be called in destructor of derived class\n";
        } catch (...) {
            //logging failed.
            //no exceptions from destructor.
        }
    }
}

void TPeriodicallyUpdated::SetReloadDuration(TDuration reloadDuration) {
    mReloadDuration = reloadDuration;
}
