#include "watchdog.h"

#include <library/cpp/watchdog/lib/factory.h>
#include <library/cpp/watchdog/lib/interface.h>

#include <util/folder/dirut.h>
#include <util/generic/singleton.h>
#include <util/stream/file.h>

void TEmergencyCgi::Set(const TString& value) {
    TGuard<TMutex> g(CgiMutex);
    Cgi = value;
    while (Cgi.size() > 0 && (Cgi.back() == ' ' || Cgi.back() == '\t') || Cgi.back() == '\n') {
        Cgi.pop_back();
    }
    IsEnabled = true;
};

bool TEmergencyCgi::GetCgi(TString& result) const {
    TGuard<TMutex> g(CgiMutex);
    if (IsEnabled) {
        result = Cgi;
        return true;
    } else {
        return false;
    }
};

void TEmergencyCgi::Disable() {
    TGuard<TMutex> g(CgiMutex);
    IsEnabled = false;
}

void TEmergencyWatchDogHandle::Check(TInstant timeNow) {
    if (timeNow - LastCheck >= Frequency) {
        if (NFs::Exists(Filename)) {
            try {
                TUnbufferedFileInput file(Filename);
                EmergencyCgi->Set(file.ReadAll());
            } catch (...) {
                EmergencyCgi->Disable();
            }
        } else {
            EmergencyCgi->Disable();
        }
        LastCheck = timeNow;
    }
}

IWatchDog* TEmergencyWatchDogHandle::Create(const TString& filename, TDuration frequency, TAtomicSharedPtr<TEmergencyCgi> emergencyCgi) {
    TWatchDogHandlePtr handlePtr(new TEmergencyWatchDogHandle(filename, frequency, emergencyCgi));
    return Singleton<TWatchDogFactory>()->RegisterWatchDogHandle(handlePtr);
}
