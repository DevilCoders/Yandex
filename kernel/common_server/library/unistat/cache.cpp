#include "cache.h"
#include <library/cpp/logger/global/global.h>
#include <library/cpp/http/client/query.h>
#include <library/cpp/monlib/encode/json/json.h>
#include <library/cpp/monlib/service/pages/registry_mon_page.h>
#include <library/cpp/http/client/client.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <library/cpp/monlib/encode/spack/spack_v1.h>

TSignalGuardAdd::TSignalGuardAdd(const TString& name, const TString& metric, const double startSignal, const double finishSignal)
    : Name(name)
    , Metric(metric)
    , FinishSignal(finishSignal)
{
    TCSSignals::SignalAdd(Name, Metric, startSignal);
}

TSignalGuardAdd::~TSignalGuardAdd() {
    TCSSignals::SignalAdd(Name, Metric, FinishSignal);
}

TSignalGuardLast::TSignalGuardLast(const TString& name, const TString& metric, const double startSignal, const double finishSignal)
    : Name(name)
    , Metric(metric)
    , FinishSignal(finishSignal)
{
    TCSSignals::SignalLastX(Name, Metric, startSignal);
}

TSignalGuardLast::~TSignalGuardLast() {
    TCSSignals::SignalLastX(Name, Metric, FinishSignal);
}

ui64 TCSSignals::GetValueRate(const NMonitoring::TLabels& l) {
    auto rate = Singleton<TCSSignals>()->GetMetricsRegistry()->Rate(l);
    if (!rate) {
        return 0;
    }
    return rate->Get();
}

ui64 TCSSignals::GetValueCounter(const NMonitoring::TLabels& l) {
    auto rate = Singleton<TCSSignals>()->GetMetricsRegistry()->Counter(l);
    if (!rate) {
        return 0;
    }
    return rate->Get();
}

ui64 TCSSignals::GetValueGauge(const NMonitoring::TLabels& l) {
    auto rate = Singleton<TCSSignals>()->GetMetricsRegistry()->Gauge(l);
    if (!rate) {
        return 0;
    }
    return rate->Get();
}

void TCSSignals::DoSignalSpec(const TString& name, const TString& metric, const double value, const EAggregationType aType, const TString& /*suffix*/, const NMonitoring::TLabels& labelsExt) {
    TReadGuard rg(Mutex);
    TString signalName = name;
    if (name && metric) {
        signalName += ".";
    }
    signalName += metric;
    TAtomicSharedPtr<NMonitoring::TMetricRegistry> mr = GetMetricsRegistry();
    if (mr) {
        NMonitoring::TLabels labels = labelsExt;
        labels.Add("sensor", signalName);

        if (aType == EAggregationType::Sum) {
            NMonitoring::TRate* metric = mr->Rate(labels);
            metric->Add(value);
        } else {
            NMonitoring::TGauge* metric = mr->Gauge(labels);
            metric->Set(value);
        }
    }
}

void TCSSignals::DoSignalHistogram(const TString& name, const TString& metric, const double value, const EAggregationType aType, const TVector<double>& intervals, const NMonitoring::TLabels& labelsExt) {
    TReadGuard rg(Mutex);
    TString signalName = name;
    if (name && metric) {
        signalName += "-";
    }
    signalName += metric;

    TAtomicSharedPtr<NMonitoring::TMetricRegistry> mr = GetMetricsRegistry();
    if (mr) {
        NMonitoring::TLabels labels = labelsExt;
        labels.Add("sensor", signalName);
        NMonitoring::IHistogram* metric = nullptr;
        if (aType == EAggregationType::Sum) {
            labels.Add("type", "rate");
            metric = mr->HistogramRate(labels, NMonitoring::ExplicitHistogram(intervals));
        } else {
            // Не рекомендуется к использованию в Solomon
            labels.Add("type", "counter");
            metric = mr->HistogramCounter(labels, NMonitoring::ExplicitHistogram(intervals));
        }
        metric->Record(value);
    }
}

void TCSSignals::RegisterMonitoringImpl(const TMonitoringConfig& config) {
    TWriteGuard wg(Mutex);
    Y_ASSERT(!Config);
    if (!!Config) {
        return;
    }
    Config = config;
    MetricsRegistryImpl = MakeAtomicShared<NMonitoring::TMetricRegistry>(Config->GetLabels());
    AtomicSet(ActiveFlag, 1);
    if (Config->GetMethod() == "push") {
        ThreadPoolPush.Start(1);
        ThreadPoolPush.SafeAddAndOwn(MakeHolder<TThreadPushAgent>(*this));
    } else if (Config->GetMethod() == "pull") {
        MonService = MakeHolder<NMonitoring::TMonService2>(Config->GetHttpOptions(), GetProgramName(), NMonitoring::CreateFakeAuth());
        MonService->Register(new NMonitoring::TMetricRegistryPage("counters", "Counters", MetricsRegistryImpl));
        MonService->Register(new NMonitoring::TVersionMonPage);
        CHECK_WITH_LOG(MonService->Start());
    }
}

void TCSSignals::UnregisterMonitoringImpl() {
    TWriteGuard wg(Mutex);
    AtomicSet(ActiveFlag, 0);
    if (MonService) {
        MonService->Stop();
        MonService.Reset(nullptr);
    } else if (!!MetricsRegistryImpl) {
        ThreadPoolPush.Stop();
    }
    MetricsRegistryImpl = nullptr;
    Config.Clear();
}

TAtomicSharedPtr<NMonitoring::TMetricRegistry> TCSSignals::GetMetricsRegistry() const {
    return MetricsRegistryImpl;
}

TCSSignals::~TCSSignals() {
    SignalCancellation.Stop();
    UnregisterMonitoring();
}

TCSSignals::TSignalBuilder TCSSignals::Signal(const TString& name, const double value /*= 1*/) {
    TSignalBuilder result(name);
    result.SetValue(value);
    return result;
}

TCSSignals::TSignalBuilder TCSSignals::LTSignal(const TString& name, const double value /*= 0*/, const TMaybe<TDuration> d /*= {}*/) {
    TSignalBuilder result(name);
    result.SetValue(value).SetType(EAggregationType::LastValue).SetValue(value);
    if (d) {
        result.SetTimeout(*d);
    } else {
        result.InitTimeout();
    }
    return result;
}

namespace NMonitoring {
    class TCorrectEncoderJson: public IMetricConsumer {
    private:
        IMetricConsumerPtr BaseConsumer;
        TMaybe<TInstant> OverridenTime;
    public:
        TCorrectEncoderJson(IMetricConsumerPtr&& baseConsumer)
            : BaseConsumer(std::move(baseConsumer)) {

        }

        void OnStreamBegin() override {
            BaseConsumer->OnStreamBegin();
        }
        void OnStreamEnd() override {
            BaseConsumer->OnStreamEnd();
        }
        void OnCommonTime(TInstant time) override {
            BaseConsumer->OnCommonTime(time);
        }
        void OnMetricBegin(EMetricType type) override {
            BaseConsumer->OnMetricBegin(type);
            if (type == EMetricType::HIST_RATE || type == EMetricType::RATE) {
                OverridenTime = TInstant::Zero();
            }
        }
        void OnMetricEnd() override {
            BaseConsumer->OnMetricEnd();
            OverridenTime.Clear();
        }

        void OnLabelsBegin() override {
            BaseConsumer->OnLabelsBegin();
        }
        void OnLabelsEnd() override {
            BaseConsumer->OnLabelsEnd();
        }
        void OnLabel(TStringBuf name, TStringBuf value) override {
            BaseConsumer->OnLabel(name, value);
        }
        void OnDouble(TInstant time, double value) override {
            BaseConsumer->OnDouble(OverridenTime.GetOrElse(time), value);
        }
        void OnInt64(TInstant time, i64 value) override {
            BaseConsumer->OnInt64(OverridenTime.GetOrElse(time), value);
        }
        void OnUint64(TInstant time, ui64 value) override {
            BaseConsumer->OnUint64(OverridenTime.GetOrElse(time), value);
        }
        void OnHistogram(TInstant time, IHistogramSnapshotPtr snapshot) override {
            BaseConsumer->OnHistogram(OverridenTime.GetOrElse(time), snapshot);
        }
        void OnLogHistogram(TInstant time, TLogHistogramSnapshotPtr snapshot) override {
            BaseConsumer->OnLogHistogram(OverridenTime.GetOrElse(time), snapshot);
        }
        void OnSummaryDouble(TInstant time, ISummaryDoubleSnapshotPtr snapshot) override {
            BaseConsumer->OnSummaryDouble(OverridenTime.GetOrElse(time), snapshot);
        }
    };

}

void TCSSignals::TThreadPushAgent::Process(void* /*threadSpecificResource*/) {
    while (Owner.IsActive()) {
        TString data;
        {
            TStringOutput out(data);
            NMonitoring::TCorrectEncoderJson encoder(NMonitoring::EncoderSpackV1(&out, NMonitoring::ETimePrecision::SECONDS, NMonitoring::ECompression::IDENTITY));
            TReadGuard rg(Owner.Mutex);
            if (!!Owner.MetricsRegistryImpl) {
                const TInstant startInstant = Now();
                Owner.MetricsRegistryImpl->Accept(startInstant - TDuration::MicroSeconds(startInstant.MicroSeconds() % Owner.Config->GetPushPeriod().MicroSeconds()), &encoder);
            }
        }

        NHttp::TFetchQuery req(Owner.Config->GetPushUrl(),
            NHttp::TFetchOptions()
            .SetTimeout(Owner.Config->GetPushTimeout())
            .SetPostData(data)
            .SetContentType(::ToString(NMonitoring::NFormatContenType::SPACK))
        );
        NHttp::Fetch(req);
        Sleep(Min(TDuration::Seconds(15), Owner.Config->GetPushPeriod()));
    }
}

void TMonitoringConfig::Init(const TYandexConfig::Section* section) {
    Method = section->GetDirectives().Value("Method", Method);

    HttpOptions.Port = section->GetDirectives().Value("Port", HttpOptions.Port);
    HttpOptions.Host = section->GetDirectives().Value<TString>("Host", "");
    HttpOptions.nThreads = section->GetDirectives().Value("nThreads", 16);

    PushUrl = section->GetDirectives().Value("PushUrl", PushUrl);
    PushTimeout = section->GetDirectives().Value("PushTimeout", PushTimeout);
    PushPeriod = section->GetDirectives().Value("PushPeriod", PushPeriod);

    auto sections = section->GetAllChildren();
    auto it = sections.find("Labels");
    if (it != sections.end()) {
        for (auto&& i : it->second->GetDirectives()) {
            Labels.Add(i.first, i.second);
        }
    }
    AssertCorrectConfig(HttpOptions.Port || PushUrl, "Incorrect monitoring configuration");
}

void TMonitoringConfig::ToString(IOutputStream& os) const {
    os << "Method: " << Method << Endl;

    os << "Port: " << HttpOptions.Port << Endl;
    os << "Host: " << HttpOptions.Host << Endl;
    os << "nThreads: " << HttpOptions.nThreads << Endl;

    os << "PushUrl: " << PushUrl << Endl;
    os << "PushTimeout: " << PushTimeout << Endl;
    os << "PushPeriod: " << PushPeriod << Endl;

    os << "<Labels>" << Endl;
    for (auto&& i : Labels) {
        os << i.Name() << ": " << i.Value() << Endl;
    }
    os << "</Labels>" << Endl;
}

TCSSignals::TSignalBuilder& TCSSignals::TSignalBuilder::InitTimeout() {
    Timeout = Max<TDuration>(TDuration::Seconds(30), Singleton<TCSSignals>()->GetPingPeriod()) * 1.1;
    return *this;
}

TCSSignals::TSignalBuilder::TSignalBuilder(TSignalBuilder&& item) {
    item.Release();
    InitFrom(item);
}

TCSSignals::TSignalBuilder::~TSignalBuilder() {
    try {
        if (Released) {
            return;
        }
        if (Intervals) {
            Singleton<TCSSignals>()->DoSignalHistogram(Name, "", Value, Type, *Intervals, Labels);
        } else {
            Singleton<TCSSignals>()->DoSignalSpec(Name, "", Value, Type, "", Labels);
        }
        if (Timeout && Type == EAggregationType::LastValue) {
            Singleton<TCSSignals>()->RegisterSignalCancellation(std::move(this->SetValue(0)));
        }
    } catch (...) {
        ALERT_LOG << "Problems with signal: " << Name << ": " << CurrentExceptionMessage() << Endl;
        Y_ASSERT(false);
    }
}
