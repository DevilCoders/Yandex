#include "dynamic_counters.h"

namespace {
    template<class Type, bool Derivative>
    class TCounterMetric : public NSFStats::TBasicMetric {
    public:
        using TBasicProxy = NSFStats::TStats::TContext::TBasicProxy<TCounterMetric>;
        using TBasicMetric::TBasicMetric;

        template<class TBase = TBasicProxy>
        struct TProxy : TBase {
            using TBase::TBase;

            void Set(Type v) {
                this->AddFunc([v](TCounterMetric* metric) {
                    metric->Set(v);
                });
            }
        };

        void Set(Type value) { Value = value; }

        void Visit(TFunc&& func) noexcept override {
            func(this->Name(), Value, Derivative);
        }

    private:
        Type Value{};
    };

    class THistogramSnapshotWrapper : public NSFStats::TBasicMetric {
    public:
        using TBasicProxy = NSFStats::TStats::TContext::TBasicProxy<THistogramSnapshotWrapper>;
        using TBasicMetric::TBasicMetric;

        template<class TBase = TBasicProxy>
        struct TProxy : TBase {
            using TBase::TBase;

            void Set(NMonitoring::IHistogramSnapshotPtr v) {
                this->AddFunc([v = std::move(v)](THistogramSnapshotWrapper* metric) {
                    metric->Set(std::move(v));
                });
            }
        };

        void Set(NMonitoring::IHistogramSnapshotPtr v) {
            Snapshot = std::move(v);
        }

        void Visit(TFunc&& func) noexcept override {
            for (ui32 i = 0, cnt = Snapshot->Count(); i != cnt; ++i) {
                auto value = Snapshot->Value(i);
                auto upperBound = Snapshot->UpperBound(i);
                TString name = "inf";
                if (upperBound != NMonitoring::HISTOGRAM_INF_BOUND) {
                    name = ToString(upperBound);
                }

                func(GetFullName(name), value, true);
            }
        }

    private:
        TString GetFullName(const TString& name) const {
            return TStringBuilder{} << this->Name()
                << NSFStats::SOLOMON_LABEL_DELIMITER << "t"
                << NSFStats::SOLOMON_VALUE_DELIMITER << name;
        }

        NMonitoring::IHistogramSnapshotPtr Snapshot;
    };

    class TConverter : public NMonitoring::ICountableConsumer {
    public:
        explicit TConverter(NSFStats::TStats& service, THashMap<TString, THashSet<TString>> labelsFilter)
            : Ctx(service)
            , LabelsFilter(std::move(labelsFilter))
        {}

        void ConvertGroup(TString labelPath, const NMonitoring::TDynamicCounters& counters) {
            LabelPaths.push_back(std::move(labelPath));

            auto snapshot = counters.ReadSnapshot();

            for (const auto& [label, value]: snapshot) {
                value->Accept(label.LabelName, label.LabelValue, *this);
            }

            LabelPaths.pop_back();
        }

        bool CheckLabelsFilter(const TString& labelName, const TString& labelValue) {
            if (LabelsFilter.contains(labelName) && !LabelsFilter[labelName].contains(labelValue)) {
                return false; // filtered out
            }
            return true;
        }

        void OnCounter(
            const TString& labelName, const TString& labelValue,
            const NMonitoring::TCounterForPtr* counter) override
        {
            using TNormalMetric = TCounterMetric<ui64, false>;
            using TDerivativeMetric = TCounterMetric<ui64, true>;

            if (!CheckLabelsFilter(labelName, labelValue)) {
                return;
            }
            auto metricName = MakeMetricName(labelName, labelValue);
            if (counter->ForDerivative()) {
                Ctx.Get<TDerivativeMetric>(metricName).Set(counter->Val());
            } else {
                Ctx.Get<TNormalMetric>(metricName).Set(counter->Val());
            }
        }

        void OnHistogram(
            const TString& labelName, const TString& labelValue,
            const NMonitoring::IHistogramSnapshotPtr snapshot, bool) override
        {
            if (!CheckLabelsFilter(labelName, labelValue)) {
                return;
            }
            auto metricName = MakeMetricName(labelName, labelValue);
            Ctx.Get<THistogramSnapshotWrapper>(metricName).Set(snapshot);
        }

        virtual void OnGroupBegin(
            const TString& labelName, const TString& labelValue,
            const NMonitoring::TDynamicCounters* /*group*/) override
        {
            // TODO: support "CheckLabelsFilter" via "GroupFilteredOut" flag
            LabelPaths.push_back(MakeMetricName(labelName, labelValue));
        }

        virtual void OnGroupEnd(
            const TString& /*labelName*/, const TString& /*labelValue*/,
            const NMonitoring::TDynamicCounters* /*group*/) override
        {
            LabelPaths.pop_back();
        }

    private:
        TString MakeMetricName(const TString& labelName, const TString& labelValue) {
            return TStringBuilder() << LabelPaths.back()
                << labelName << NSFStats::SOLOMON_VALUE_DELIMITER
                << labelValue << NSFStats::SOLOMON_LABEL_DELIMITER;
        }

        NSFStats::TStats::TContext Ctx;
        THashMap<TString, THashSet<TString>> LabelsFilter;
        TVector<TString> LabelPaths;
    };
}

void NSFStats::FromDynamicCounters(
    const NMonitoring::TDynamicCounters& counters,
    TStats& service,
    TString labelPath,
    THashMap<TString, THashSet<TString>> labelsFilter)
{
    TConverter converter(service, std::move(labelsFilter));
    converter.ConvertGroup(std::move(labelPath), counters);
}
