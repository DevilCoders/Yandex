#pragma once
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/system/rwlock.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/unistat/types.h>
#include <library/cpp/monlib/metrics/histogram_collector.h>
#include <library/cpp/monlib/service/monservice.h>
#include <library/cpp/monlib/service/pages/version_mon_page.h>
#include <library/cpp/monlib/dynamic_counters/page.h>
#include <library/cpp/monlib/metrics/metric_registry.h>
#include <library/cpp/http/server/options.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/yconf/conf.h>
#include <library/cpp/messagebus/scheduler/scheduler.h>

class TMonitoringConfig {
private:
    CSA_DEFAULT(TMonitoringConfig, THttpServerOptions, HttpOptions);
    CSA_READONLY_DEF(TString, PushUrl);
    CSA_READONLY(TDuration, PushTimeout, TDuration::MilliSeconds(50));
    CSA_READONLY(TDuration, PushPeriod, TDuration::Seconds(5));
    CSA_READONLY(TString, Method, "pull");
    CSA_READONLY_DEF(NMonitoring::TLabels, Labels);
public:
    TMonitoringConfig() = default;
    void Init(const TYandexConfig::Section* section);

    void ToString(IOutputStream& os) const;
};

class TSignalTagsSet {
private:
    mutable TMaybe<TString> Key;
    TMap<TString, TString> Tags;

    const TString& GetKey() const {
        if (!Key) {
            TString key;
            for (auto&& [k, v] : Tags) {
                key += k + ":" + v + ";";
            }
            Key = key;
        }
        return *Key;
    }
public:
    template <class T>
    TSignalTagsSet& operator()(const TString& name, const T& value) {
        return AddTag(name, value);
    }

    TMap<TString, TString>::const_iterator begin() const {
        return Tags.begin();
    }

    TMap<TString, TString>::const_iterator end() const {
        return Tags.end();
    }

    template <class T>
    TSignalTagsSet& AddTag(const TString& name, const T& value) {
        Key.Clear();
        Tags.emplace(name, ::ToString(value));
        return *this;
    }

    template <class T>
    TSignalTagsSet& Force(const TString& name, const T& value) {
        Key.Clear();
        TString valueStr = ::ToString(value);
        auto itInfo = Tags.emplace(name, valueStr);
        if (!itInfo.second) {
            itInfo.first->second = std::move(valueStr);
        }
        return *this;
    }

    bool operator<(const TSignalTagsSet& item) const {
        return GetKey() < item.GetKey();
    }
};

class TCSSignals {
public:
    class TSignalBuilder: TMoveOnly {
    private:
        bool Released = false;
        NMonitoring::TLabels Labels;
        TMaybe<TDuration> Timeout;
        CSA_MAYBE(TSignalBuilder, TVector<double>, Intervals);
        CSA_DEFAULT(TSignalBuilder, TString, Name);
        CS_ACCESS(TSignalBuilder, double, Value, 1);
        CS_ACCESS(TSignalBuilder, EAggregationType, Type, EAggregationType::Sum);
        void InitFrom(TSignalBuilder& item) {
            Released = false;
            Labels = std::move(item.Labels);
            Timeout = std::move(item.Timeout);
            Intervals = std::move(item.Intervals);
            Name = std::move(item.Name);
            Value = std::move(item.Value);
            Type = std::move(item.Type);
        }
    public:

        TSignalBuilder& operator++() {
            Value += 1;
            return *this;
        }

        TSignalBuilder& operator()(const double value) {
            return SetValue(value);
        }

        TSignalBuilder& operator()(const TSignalTagsSet& tags) {
            for (auto&& [k, v] : tags) {
                AddLabel(k, v);
            }
            return *this;
        }

        TSignalBuilder& operator()(const TDuration d) {
            return SetValue(d.MilliSeconds());
        }

        void DropTimeout() {
            Timeout.Clear();
        }

        void Release() {
            Released = true;
        }

        TString GetSignalId() const {
            return Name + ::ToString(Labels.Hash());
        }

        template <class T>
        TSignalBuilder& AddLabel(const TString& name, const T& value) {
            Labels.Add(name, ::ToString(value));
            return *this;
        }

        TDuration GetTimeout() const {
            return Timeout.GetOrElse(TDuration::Max());
        }

        TSignalBuilder& SetTimeout(const TDuration d) {
            Timeout = d;
            return *this;
        }

        TSignalBuilder& InitTimeout();

        TSignalBuilder& SetLabels(const NMonitoring::TLabels& labels) {
            Labels = labels;
            return *this;
        }

        TSignalBuilder(const TSignalBuilder& item) = delete;
        TSignalBuilder(TSignalBuilder& item) = delete;
        TSignalBuilder(TSignalBuilder&& item);

        TSignalBuilder(const TString& name)
            : Name(name) {
        }

        ~TSignalBuilder();

        TSignalBuilder& SetBuckets(const TVector<double>& intervals) {
            Intervals = intervals;
            return*this;
        }

        template <class T>
        TSignalBuilder& operator()(const TString& name, const T& value) {
            return AddLabel(name, value);
        }
    };

    static ui64 GetValueRate(const NMonitoring::TLabels& l);
    static ui64 GetValueCounter(const NMonitoring::TLabels& l);
    static ui64 GetValueGauge(const NMonitoring::TLabels& l);

private:

    class TThreadPushAgent: public IObjectInQueue {
    private:
        TCSSignals& Owner;
    public:
        TThreadPushAgent(TCSSignals& owner)
            : Owner(owner)
        {

        }

        virtual void Process(void* threadSpecificResource) override;
    };

    TAtomic ActiveFlag = 0;
    TMaybe<TMonitoringConfig> Config;
    TAtomicSharedPtr<NMonitoring::TMetricRegistry> MetricsRegistryImpl;

    TRWMutex CancellationMutex;
    TMap<TString, TAtomic> SignalCancellationCounter;
    NBus::NPrivate::TScheduler SignalCancellation;

    TRWMutex Mutex;
    THolder<NMonitoring::TMonService2> MonService;

    TThreadPool ThreadPoolPush;

    void DoSignalLabels(const TString& name, const NMonitoring::TLabels& labels, const double value, EAggregationType aType);
    void DoSignalSpec(const TString& name, const TString& metric, const double value, const EAggregationType aType, const TString& /*suffix*/, const NMonitoring::TLabels& labels);
    void DoSignalHistogram(const TString& name, const TString& metric, const double value, const EAggregationType aType, const TVector<double>& intervals, const NMonitoring::TLabels& labels);
    void RegisterMonitoringImpl(const TMonitoringConfig& config);
    void UnregisterMonitoringImpl();

    TAtomicSharedPtr<NMonitoring::TMetricRegistry> GetMetricsRegistry() const;

    class TSignalCancellation: public NBus::NPrivate::IScheduleItem {
    private:
        TCSSignals& Owner;
        THolder<TSignalBuilder> Builder;
    public:
        TSignalCancellation(TSignalBuilder&& signal, TCSSignals& owner)
            : NBus::NPrivate::IScheduleItem(Now() + signal.GetTimeout())
            , Owner(owner)
        {
            Builder = MakeHolder<TSignalBuilder>(std::move(signal));
            Builder->DropTimeout();
        }

        virtual void Do() override {
            if (!Builder) {
                return;
            }
            if (!Owner.UnregisterSignalCancellation(*Builder)) {
                Builder->Release();
            }
            Builder.Destroy();
        }
    };

    bool UnregisterSignalCancellation(const TSignalBuilder& sBuilder) {
        TReadGuard rg(CancellationMutex);
        auto it = SignalCancellationCounter.find(sBuilder.GetSignalId());
        if (it != SignalCancellationCounter.end()) {
            if (!AtomicDecrement(it->second)) {
                rg.Release();
                TWriteGuard rg(CancellationMutex);
                auto it = SignalCancellationCounter.find(sBuilder.GetSignalId());
                if (it != SignalCancellationCounter.end() && !AtomicGet(it->second)) {
                    SignalCancellationCounter.erase(it);
                    return true;
                }
            }
        }
        return false;
    }

    void RegisterSignalCancellation(TSignalBuilder&& sBuilder) {
        if (sBuilder.GetTimeout() != TDuration::Max()) {
            {
                TReadGuard rg(CancellationMutex);
                auto it = SignalCancellationCounter.find(sBuilder.GetSignalId());
                if (it != SignalCancellationCounter.end()) {
                    AtomicIncrement(it->second);
                } else {
                    rg.Release();
                    TWriteGuard rg(CancellationMutex);
                    AtomicIncrement(SignalCancellationCounter[sBuilder.GetSignalId()]);
                }
            }
            SignalCancellation.Schedule(new TSignalCancellation(std::move(sBuilder), *this));
        }
    }

public:

    TDuration GetPingPeriod() const {
        if (Config) {
            return Config->GetPushPeriod();
        } else {
            return TDuration::Seconds(5);
        }
    }

    bool IsActive() const {
        return AtomicGet(ActiveFlag);
    }

    ~TCSSignals();

    template <class TContainer>
    static TString GetBucketId(const TContainer& borders, const typename TContainer::value_type& value) {
        TString bucketId;
        auto it = borders.lower_bound(value);
        if (it == borders.end()) {
            bucketId = ::ToString(*borders.rbegin()) + "-inf";
        } else {
            auto itNext = it;
            ++itNext;
            if (itNext == borders.end()) {
                bucketId = ::ToString(*it) + "-inf";
            } else {
                bucketId = ::ToString(*it) + "-" + ::ToString(*itNext);
            }
        }
        return bucketId;
    }

    static TSignalBuilder Signal(const TString& name, const double value = 1);

    static TSignalBuilder SignalProblem(const TString& name, const double value = 1) {
        return std::move(Signal(name, value)("category", "problem"));
    }

    static TSignalBuilder LSignal(const TString& name, const double value = 0) {
        return std::move(Signal(name).SetType(EAggregationType::LastValue).SetValue(value));
    }

    static TSignalBuilder HSignal(const TString& name, const TVector<double>& intervals) {
        return std::move(Signal(name).SetType(EAggregationType::Sum).SetBuckets(intervals));
    }

    static TSignalBuilder HSignalCounter(const TString& name, const TVector<double>& intervals) {
        return std::move(Signal(name).SetType(EAggregationType::LastValue).SetBuckets(intervals));
    }

    static TSignalBuilder LSignalProblem(const TString& name, const double value = 0) {
        return std::move(LSignal(name).SetValue(value)("category", "problem"));
    }

    static TSignalBuilder LTSignal(const TString& name, const double value = 0, const TMaybe<TDuration> d = {});

    static TSignalBuilder LTSignalProblem(const TString& name, const double value = 0, const TMaybe<TDuration> d = {}) {
        return std::move(LTSignal(name, value, d)("category", "problem"));
    }

    static void RegisterMonitoring(const TMonitoringConfig& config) {
        Singleton<TCSSignals>()->RegisterMonitoringImpl(config);
    }

    static void UnregisterMonitoring() {
        Singleton<TCSSignals>()->UnregisterMonitoringImpl();
    }

    static void SignalLastX(const TString& name, const TString& metric, const double value) {
        SignalSpec(name, metric, value, EAggregationType::LastValue, "axxx");
    }

    static void SignalSpec(const TString& name, const TString& metric, const double value, EAggregationType aType, const TString& suffix, const NMonitoring::TLabels& labels = NMonitoring::TLabels()) {
        Singleton<TCSSignals>()->DoSignalSpec(name, metric, value, aType, suffix, labels);
    }

    static void SignalAdd(const TString& name, const TString& metric, const double value) {
        SignalSpec(name, metric, value, EAggregationType::Sum, "dmmm");
    }

    static void SignalAddX(const TString& name, const TString& metric, const double value) {
        SignalSpec(name, metric, value, EAggregationType::Sum, "dmmx");
    }

    static void SignalMaxX(const TString& name, const TString& metric, const double value) {
        SignalSpec(name, metric, value, EAggregationType::Max, "axxx");
    }

    static void SignalHistogram(const TString& name, const TString& metric, const double value, const TVector<double>& intervals, const NMonitoring::TLabels& labels = NMonitoring::TLabels()) {
        Singleton<TCSSignals>()->DoSignalHistogram(name, metric, value, EAggregationType::Sum, intervals, labels);
    }

};

class TSignalGuardAdd {
private:
    const TString Name;
    const TString Metric;
    const double FinishSignal;
public:
    TSignalGuardAdd(const TString& name, const TString& metric, const double startSignal, const double finishSignal);

    ~TSignalGuardAdd();
};

class TSignalGuardLast {
private:
    const TString Name;
    const TString Metric;
    double FinishSignal;
public:
    TSignalGuardLast(const TString& name, const TString& metric, const double startSignal, const double finishSignal);

    ~TSignalGuardLast();
};

template <>
class TSingletonTraits<TCSSignals> {
public:
    static const size_t Priority = 100;
};
