#include "watchdog.h"

#include <library/cpp/watchdog/lib/factory.h>

#include <library/cpp/http/io/stream.h>

#include <util/generic/singleton.h>
#include <util/network/socket.h>

bool TPortCheckWatchDogHandle::CheckPort(ui16 port, TDuration timeout) {
    try {
        TSocket s(TNetworkAddress(port), timeout);
        s.SetSocketTimeout(1, timeout.MilliSeconds());
        return true;
    } catch (...) {
        return false;
    }
}

bool TPortCheckWatchDogHandle::CheckRequest(ui16 port, const TString& request, TDuration timeout) {
    try {
        TSocket s(TNetworkAddress(port), timeout);
        s.SetSocketTimeout(1, timeout.MilliSeconds());
        TSocketOutput so(s);
        so << "GET /" << request << " HTTP/1.1\r\n\r\n";
        so.Flush();
        TSocketInput si(s);
        THttpInput hi(&si);
        int ret = ParseHttpRetCode(hi.FirstLine());
        if (ret / 100 > 2)
            ythrow yexception() << "Invalid http response " << ret << ": " << hi.ReadAll();
        return true;
    } catch (...) {
        return false;
    }
}

void TPortCheckWatchDogHandle::DoCheck(TInstant /*timeNow*/) {
    bool checkResult = true;
    if (!!Settings.Request) {
        checkResult = CheckRequest(Settings.Port, Settings.Request, Settings.Timeout);
    } else {
        checkResult = CheckPort(Settings.Port, Settings.Timeout);
    }
    if (!checkResult) {
        ++FailsCounter;
        if (FailsCounter > Settings.LimitCheckFail) {
            Cerr << "Watchdog fired for port: " << Settings.Port << Endl;
            Y_FAIL();
        }
    } else {
        FailsCounter = 0;
    }
}
