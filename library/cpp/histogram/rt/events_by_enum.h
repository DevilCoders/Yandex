#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <library/cpp/unistat/unistat.h>
#include "time_slide.h"

template <class T>
class TMaxEventDefault {
public:
    static ui32 MaxEvent() {
        return (ui32)T::COUNT;
    }
};

template <class T, class TMaxEvent = TMaxEventDefault<T>>
class TEventsMeter {
private:
    TTimeSlidedHistogram Histogram;
    TString Suffix;
    TVector<TString> SignalNames;
    TVector<TString> CTypeSignalNames;

    const TString& GetSignalName(const T value) const {
        return SignalNames[(ui32)value];
    }

    const TString& GetCTypeSignalName(const T value) const {
        return CTypeSignalNames[(ui32)value];
    }

public:
    void Init(TUnistat& unistat) const {
        for (auto&& i : SignalNames) {
            unistat.DrillFloatHole(i, "dmmm", NUnistat::TPriority(50));
        }
        for (auto&& i : CTypeSignalNames) {
            unistat.DrillFloatHole(i, "dmmm", NUnistat::TPriority(50));
        }
    }

public:
    TEventsMeter(const ui32 segmentsCount, const ui32 secondsDuration, const TString& name = "")
        : Histogram(segmentsCount, secondsDuration, 0, TMaxEvent::MaxEvent(), TMaxEvent::MaxEvent())
    {
        Suffix = (!!name ? ("-" + name) : "");
        for (ui32 i = 0; i < TMaxEvent::MaxEvent(); ++i) {
            SignalNames.push_back("events-CTYPE-SERV-" + TypeName<T>() + "-" + ToString(T(i)) + Suffix);
        }
        for (ui32 i = 0; i < TMaxEvent::MaxEvent(); ++i) {
            CTypeSignalNames.push_back("events-CTYPE-" + TypeName<T>() + "-" + ToString(T(i)) + Suffix);
        }
        Init(TUnistat::Instance());
    }

    void AddEvent(const T value, const double signalValue = 1) {
        Histogram.Add((ui32)value);
        auto& tass = TUnistat::Instance();
        if (!tass.PushSignalUnsafe(GetSignalName(value), signalValue)) {
            DEBUG_LOG << "Not initialized signal: " << GetSignalName(value) << Endl;
            tass.PushSignalUnsafe("debug-errors-CTYPE-SERV", 1);
        }
        if (!tass.PushSignalUnsafe(GetCTypeSignalName(value), signalValue)) {
            DEBUG_LOG << "Not initialized signal: " << GetCTypeSignalName(value) << Endl;
            tass.PushSignalUnsafe("debug-errors-CTYPE", 1);
        }
    }

    TVector<ui32> GetState() const {
        return Histogram.Clone();
    }

    void Clear() {
        Histogram.Clear();
    }

    NJson::TJsonValue GetReport() const {
        NJson::TJsonValue report(NJson::JSON_MAP);
        TVector<ui32> result = Histogram.Clone();
        CHECK_WITH_LOG(result.size() == TMaxEvent::MaxEvent() + 2);
        CHECK_WITH_LOG(result.front() == 0);
        CHECK_WITH_LOG(result.back() == 0);
        for (ui32 i = 1; i < result.size() - 1; ++i) {
            report[ToString(T(i - 1))] = result[i];
        }
        return report;
    }
};
