#pragma once

#include <library/cpp/watchdog/lib/handle.h>
#include <library/cpp/watchdog/lib/interface.h>

class TPortCheckWatchDogHandle: public IWatchDogHandleFreq {
public:
    struct TCheckerSettings {
        ui32 CheckIntervalSeconds;
        ui16 Port;
        ui32 LimitCheckFail;
        TString Request;
        TDuration Timeout = TDuration::MilliSeconds(500);
    };

    TPortCheckWatchDogHandle(const TCheckerSettings& settings)
        : IWatchDogHandleFreq(settings.CheckIntervalSeconds)
        , Settings(settings)
    {
    }

    void DoCheck(TInstant timeNow) override;

private:
    ui32 FailsCounter = 0;
    const TCheckerSettings Settings;
    bool CheckPort(ui16 port, TDuration timeout);
    bool CheckRequest(ui16 port, const TString& request, TDuration timeout);
};
