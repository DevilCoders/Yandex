#pragma once

#include <library/cpp/watchdog/lib/handle.h>

#include <util/generic/string.h>
#include <util/system/mutex.h>

class IWatchDog;

class TEmergencyCgi {
public:
    void Set(const TString& value);
    bool GetCgi(TString& result) const;
    void Disable();

private:
    TString Cgi;
    bool IsEnabled = false;
    TMutex CgiMutex;
};

class TEmergencyWatchDogHandle: public IWatchDogHandle {
public:
    TEmergencyWatchDogHandle(const TString& filename, TDuration frequency, TAtomicSharedPtr<TEmergencyCgi> emergencyCgi)
        : Filename(filename)
        , Frequency(frequency)
        , LastCheck(TInstant::Now())
        , EmergencyCgi(emergencyCgi)
    {
    }

    void Check(TInstant timeNow) override;

    static IWatchDog* Create(const TString& filename, TDuration frequency, TAtomicSharedPtr<TEmergencyCgi> emergencyCgi);

private:
    TString Filename;
    TDuration Frequency;
    TInstant LastCheck;
    TAtomicSharedPtr<TEmergencyCgi> EmergencyCgi;
};
