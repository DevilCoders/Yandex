#include "signals.h"
#include <library/cpp/logger/global/global.h>

#define BASE_PRIORITY 50
const TString ALIVE_SIGNAL_NAME = "i-am-alive";
const int ALIVE_SIGNAL_PRIORITY = 100000;

TCommonProxySignals::TCommonProxySignals(const TProcessors& processors)
    : Processors(processors)
{}

const TUnistatSignalsConfig*& TCommonProxySignals::MutableConfigPtr() {
    return *Singleton<const TUnistatSignalsConfig*>();
}

void TCommonProxySignals::Init(TUnistat& t) const {
    for (auto&& proc : Processors) {
        proc->RegisterSignals(t);
    }
    t.DrillFloatHole("debug-errors-CTYPE", "dmmm", NUnistat::TPriority(BASE_PRIORITY));
    t.DrillFloatHole(GetSignalName("", ALIVE_SIGNAL_NAME), "ammn", NUnistat::TPriority(ALIVE_SIGNAL_PRIORITY), NUnistat::TStartValue(0), EAggregationType::LastValue);
}

void TCommonProxySignals::BuildSignals(const TProcessors& processors, const TUnistatSignalsConfig& config){
    MutableConfigPtr() = &config;
    TCommonProxySignals signals(processors);
    DEBUG_LOG << "Init signals..." << Endl;
    signals.Init(TUnistat::Instance());
    DEBUG_LOG << "Init signals... ok" << Endl;
}


TString TCommonProxySignals::GetSignalName(const TString& processor, const TString& signalName) {
    return MutableConfigPtr()->GetSignalPrefix() + processor + "_" + signalName;
}


void TCommonProxySignals::PushSpecialSignal(const TString& processor, const TString& signalName, double value) {
    TUnistat& inst = TUnistat::Instance();

    const TString fullSignalName = GetSignalName(processor, signalName);

    if (!inst.PushSignalUnsafe(fullSignalName, value)) {
        DEBUG_LOG << "Not initialized signal: " << fullSignalName << Endl;
        inst.PushSignalUnsafe("debug-errors-CTYPE", 1);
    }
}


void TCommonProxySignals::PushAliveSignal(bool alive) {
    PushSpecialSignal("", ALIVE_SIGNAL_NAME, alive);
}

// maximum bucket count is 50. Limit is taken from solomon-docs: https://wiki.yandex-team.ru/solomon/agent/modules/#unistat-pull
const NUnistat::TIntervals TCommonProxySignals::TimeIntervals {0, 5, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100,
150, 200, 250, 300, 350, 400, 500, 600, 700, 800, 900, 1000, 1500, 2000, 3000, 5000, 10000, 15000,
20000, 25000, 40000, 55000, 85000, 100000, 150000, 200000,
300000, 500000, 700000, 1000000, 1500000, 2000000, 3000000};

void TUnistatSignalsConfig::ToString(IOutputStream& so) const {
    so << "Prefix: " << (SignalPrefix ? SignalPrefix : "EMPTY") << Endl;
}

void TUnistatSignalsConfig::Init(const TYandexConfig::Section& section) {
    const TYandexConfig::Directives& dir = section.GetDirectives();
    dir.GetValue("Prefix", SignalPrefix);
    if (SignalPrefix == "EMPTY")
        SignalPrefix.clear();
}
