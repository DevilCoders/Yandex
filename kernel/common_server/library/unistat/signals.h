#pragma once

#include <library/cpp/logger/global/global.h>
#include <util/generic/map.h>
#include <util/generic/serialized_enum.h>
#include "cache.h"

namespace NRTProcHistogramSignals {
    const TVector<double> IntervalsRTLineReply = { 0, 1, 2, 3, 4, 5, 10, 15, 20, 25, 30,
        40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 225, 250, 300, 350, 400,
        500, 600, 700, 800, 900, 1000, 1500, 2000, 3000, 5000, 10000, 11000, 12000, 13000, 14000, 15000, 20000 };
    const TVector<double> IntervalsExternalAPIReply = { 5, 10, 15, 20, 30, 40, 50, 60,
        70, 80, 90, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 100000 };
    const TVector<double> IntervalsTasks = { 0, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 30000, 100000, 200000 };
    const TVector<double> IntervalsTelematicSignals = { 0, 500, 1000, 5000, 10000, 20000, 50000, 100000, 200000, 300000, 500000, 600000 };
    const TVector<double> IntervalsTrustProcessing = { 0, 500, 1000, 2000, 3000, 4000, 5000, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
        100000, 200000, 300000, 500000, 600000 };
    const TVector<double> IntervalsApproachDistance = {200, 300, 500, 700, 800, 1000, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 7000, 10000};
    const TVector<double> IntervalsApproachDuration = {60, 120, 150, 180, 240, 300, 360, 420, 480, 600, 750, 900, 1200, 1500};
    const TVector<double> IntervalsPropositionConstruction = {10, 20, 30, 45, 60, 75, 90, 120, 180, 240, 300, 360, 420, 600, 900};
    const TVector<double> IntervalsAssignmentDecisionTime = {60, 120, 180, 240, 300, 420, 540, 600, 900, 1200, 1500, 1800, 3600, 7200};
    const TVector<double> IntervalsTooLongWaits = {900, 960, 1020, 1080, 1140, 1200, 1500, 1800, 2100, 2400, 2700, 3000, 3300, 3600};
}

template <class TContext>
class IContextSignal: public TNonCopyable {
private:
    using TSelf = IContextSignal<TContext>;

public:
    using TPtr = TAtomicSharedPtr<TSelf>;

public:
    virtual ~IContextSignal() = default;
    virtual void Signal(const TContext& context) const = 0;
};

template <class T>
class TContextProcessor {
public:
    static double GetValue(const T& context) {
        return context;
    }
};

template <class EClass>
TString GetEnumClassSignalLabel() {
    TString fullClassName = typeid(EClass).name();
    TStringBuf sbCorrected = fullClassName;

    while (sbCorrected.size() && sbCorrected.front() >= '0' && sbCorrected.front() <= '9') {
        sbCorrected.Skip(1);
    }

    return TString(sbCorrected.data(), sbCorrected.size());
}

template <class TContext = double, class TContextProcessor = ::TContextProcessor<TContext>>
class TUnistatSignal: public IContextSignal<TContext> {
protected:
    TString SignalName;
    TMaybe<TVector<double>> Intervals;
    EAggregationType AggregationType = EAggregationType::Sum;
    NMonitoring::TLabels Labels;
public:
    TUnistatSignal& AddLabel(const TString& key, const TString& value) {
        Labels.Add(key, value);
        return *this;
    }

    virtual void Signal(const TContext& value) const override {
        if (Intervals) {
            TCSSignals::SignalHistogram(SignalName, "", value, *Intervals, Labels);
        } else {
            TCSSignals::SignalSpec(SignalName, "", value, AggregationType, "", Labels);
        }
    }

    TUnistatSignal(const TString& signalName, const TVector<double>& intervals = NRTProcHistogramSignals::IntervalsRTLineReply)
        : SignalName(signalName)
        , Intervals(intervals) {
    }

    TUnistatSignal(const TString& signalName, const bool absolute)
        : SignalName(signalName)
        , AggregationType(absolute ? EAggregationType::LastValue : EAggregationType::Sum) {
    }

    TUnistatSignal(const TString& signalName, const EAggregationType aggrType, const TString& /*suffix*/)
        : SignalName(signalName)
        , AggregationType(aggrType) {
    }
};

template <class TKey, class TContext>
class TSignalByKey: public IContextSignal<std::pair<TKey, TContext>> {
private:
    using TSelf = TSignalByKey<TKey, TContext>;

private:
    TMap<TKey, typename IContextSignal<TContext>::TPtr> Signals;
    typename IContextSignal<TContext>::TPtr Fallback;

public:
    TSelf& RegisterFallback(typename TUnistatSignal<TContext>::TPtr signal) {
        Fallback = signal;
        return *this;
    }

    TSelf& RegisterSignal(const TKey& key, typename TUnistatSignal<TContext>::TPtr signal) {
        Signals.emplace(key, signal);
        return *this;
    }

    virtual void Signal(const std::pair<TKey, TContext>& context) const override {
        Signal(context.first, context.second);
    }

    void Signal(const TKey& key, const TContext& context) const {
        auto it = Signals.find(key);
        if (it == Signals.end()) {
            if (Fallback) {
                Fallback->Signal(context);
            } else {
                ERROR_LOG << "Incorrect signal for key " << key << Endl;
            }
            return;
        }
        it->second->Signal(context);
    }

    bool HasKey(const TKey& key) const {
        return Signals.contains(key);
    }
};

template <class EClass, class TContext>
class TEnumContext {
private:
    EClass EnumValue;
    TContext Context;

public:
    TEnumContext(EClass enumValue, const TContext& context)
        : EnumValue(enumValue)
        , Context(context)
    {
    }

    EClass GetEnumValue() const {
        return EnumValue;
    }

    const TContext& GetContext() const {
        return Context;
    }
};

template <class EClass, class TContext, bool asInt = false>
class TEnumValueSignal: public TUnistatSignal<TContext> {
private:
    using TBase = TUnistatSignal<TContext>;
public:
    virtual ~TEnumValueSignal() = default;

    template <class... TArgs>
    TEnumValueSignal(const TString& signalName, const EClass enumValue, TArgs&&... args)
        : TBase(signalName, std::forward<TArgs>(args)...)
    {
        TBase::Labels.Add(GetEnumClassSignalLabel<EClass>(), asInt ? (::ToString((int)enumValue)) : ::ToString(enumValue));
    }
};

template <class EClass, class TContext = double, bool asInt = false>
class TEnumSignal: public IContextSignal<TEnumContext<EClass, TContext>> {
private:
    using TEnumContext = ::TEnumContext<EClass, TContext>;
    using TEnumValueSignal = ::TEnumValueSignal<EClass, TContext, asInt>;

private:
    TMap<EClass, THolder<TEnumValueSignal>> Signals;

public:
    template <class... TArgs>
    TEnumSignal(const TString& signalName, TArgs&&... args) {
        const auto& all = GetEnumNames<EClass>();
        for (auto&& i : all) {
            Signals.emplace(i.first, MakeHolder<TEnumValueSignal>(signalName, i.first, std::forward<TArgs>(args)...));
        }
    }

    TEnumSignal& AddLabel(const TString& key, const TString& value) {
        for (auto&& i : Signals) {
            i.second->AddLabel(key, value);
        }
        return *this;
    }

    virtual void Signal(const TEnumContext& context) const override {
        Signal(context.GetEnumValue(), context.GetContext());
    }

    void Signal(const EClass enumValue, const TContext& context) const {
        auto signal = Signals.find(enumValue);
        if (signal != Signals.end()) {
            CHECK_WITH_LOG(signal->second);
            signal->second->Signal(context);
        } else {
            ERROR_LOG << "Incorrect code: " << (i32)enumValue << Endl;
        }
    }
};

class TNamedSignalSimple: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TNamedSignalSimple(const TString& baseName);
};

class TNamedSignalHistogram: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TNamedSignalHistogram(const TString& baseName, const TVector<double>& intervals);
};

class TNamedSignalCustom: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TNamedSignalCustom(const TString& baseName, const EAggregationType aggrInHost, const TString& suffix);
};

template <class TEnum>
class TNamedSignalEnum: public TEnumSignal<TEnum> {
private:
    using TBase = TEnumSignal<TEnum, double>;
public:
    TNamedSignalEnum(const TString& baseName, const EAggregationType aggrInHost, const TString& suffix)
        : TBase({baseName}, aggrInHost, suffix)
    {
    }
};
