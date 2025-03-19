#include "signals.h"

#include <library/cpp/logger/global/global.h>

#include <util/string/cast.h>

void TSaasUnistatSignals::PushSignal(TUnistat& inst, const TString& signal, double value) {
    if (!inst.PushSignalUnsafe(signal, value)) {
        DEBUG_LOG << "Not initialized signal: " << signal << Endl;
        inst.PushSignalUnsafe("debug-errors", 1);
    }
}

NUnistat::TPriority TSaasUnistatSignals::Prio(const TString& code, bool isService, bool isExtraInfo) const {
    int prio = 0;
    if (code == "200" || code.find("xx") != NPOS)
        prio = 50;
    else if (code == "")
        prio = 45;
    else
        prio = 40;
    if (isService)
        prio -= 10;
    if (isExtraInfo)
        prio -= 10;
    return NUnistat::TPriority(prio);
}

void TSaasUnistatSignals::PushSignalWithCode(TUnistat& inst, const TString& signal, const TString& code, const TString& xxCode) {
    PushSignal(inst, signal + code, 1);
    PushSignal(inst, signal + xxCode, 1);
}

void TSaasProxyUnistatSignals::AddCode(const int code) {
    Codes.insert(ToString(code));
    Codes.insert(ToString(code).substr(0, 1) + "xx");
}

void TSaasProxyUnistatSignals::AddService(const TString& service) {
    Services.insert(service);
}
