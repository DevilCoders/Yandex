#pragma once

#include <yweb/fleur/util/metrics/metrics.h>

#include <util/generic/maybe.h>
#include <util/stream/output.h>

TMetrics& GetGlobalMetrics();

class TCompositeMetric {
public:
    class IMetric {
    public:
        virtual void RegisterMetric(TMetrics& metrics) const = 0;
        virtual void UnregisterMetric(TMetrics& metrics) const = 0;
        virtual ~IMetric() {}
    };

    template <class T>
    class TPart: public IMetric, public T {
    public:
        template <class... TArgs>
        TPart(TCompositeMetric* parent, TArgs... args)
            : T(args...)
            , Parent(parent)
        {
            Y_ASSERT(Parent);
            Parent->AddMetric(this);
        }
        ~TPart() override {
            Y_ASSERT(Parent);
            Parent->RemoveMetric(this);
        }

        TPart<T>& operator=(const T& other) {
            T::operator=(other);
            return *this;
        }
    protected:
        virtual void RegisterMetric(TMetrics& metrics) const final {
            T::Register(metrics);
        }
        virtual void UnregisterMetric(TMetrics& metrics) const final {
            T::Deregister(metrics);
        }
    private:
        TCompositeMetric* Parent;
    };

public:
    virtual ~TCompositeMetric() {}

    void Register(TMetrics& metrics) const;
    void Deregister(TMetrics& metrics) const;

    void Register(TMetrics* metrics) const {
        if (metrics) {
            Register(*metrics);
        }
    }
    void Deregister(TMetrics* metrics) const {
        if (metrics) {
            Deregister(*metrics);
        }
    }

protected:
    void AddMetric(const IMetric* metric);
    void RemoveMetric(const IMetric* metric);

private:
    TVector<const IMetric*> Metrics;
};

template <class T>
class TAutoGlobal: public T {
public:
    template <class... TArgs>
    TAutoGlobal(TArgs... args)
        : T(args...)
    {
        T::Register(GetGlobalMetrics());
    }

    ~TAutoGlobal() {
        T::Deregister(GetGlobalMetrics());
    }
};

class TMetricsController : public TMetrics {
public:
    TMetricsController()
       : TMetrics(150, 2) // Get values every 2 seconds, hold last 150 readings (which makes it 5 minutes) to have some headroom for incoming clients like zabbix
    {

    }
    virtual void Report() override {
        // TODO: We can output metric readings to a log here (probably separate log makes sense)
    }
};

const TString& GetMetricsPrefix();
void SetMetricsPrefix(const TString& prefix);

size_t GetMetricsMaxAgeDays();
void SetMetricsMaxAgeDays(size_t age);

void CollectMetrics(IOutputStream& out);
TMaybe<TMetricResult> GetMetricResult(const TString& name, const TMetrics& host = GetGlobalMetrics());
TMaybe<TMetricResult> GetMetricResult(const TOrangeMetric& metric, const TMetrics& host = GetGlobalMetrics());
