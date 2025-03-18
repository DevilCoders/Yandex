package solomon

import (
	"sync"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/internal/pkg/registryutil"
)

// base implementation of vector of metrics of any supported type
type metricsVector struct {
	labels    []string
	metrics   sync.Map
	rated     bool
	newMetric func(map[string]string) Metric
}

// vector Metric with initializer
type vecMetric struct {
	Metric
	init sync.Once
}

func (v *metricsVector) with(tags map[string]string) Metric {
	hv, err := registryutil.VectorHash(tags, v.labels)
	if err != nil {
		panic(err)
	}

	val, _ := v.metrics.LoadOrStore(hv, new(vecMetric))

	cnt := val.(*vecMetric)
	cnt.init.Do(func() {
		cnt.Metric = v.newMetric(tags)
	})

	return cnt.Metric
}

var _ metrics.CounterVec = (*CounterVec)(nil)

// CounterVec stores counters and
// implements metrics.CounterVec interface
type CounterVec struct {
	vec *metricsVector
}

// CounterVec creates a new counters vector with given metric name and
// partitioned by the given label names.
func (r *Registry) CounterVec(name string, labels []string) metrics.CounterVec {
	var vec *metricsVector
	vec = &metricsVector{
		labels: append([]string(nil), labels...),
		rated:  r.rated,
		newMetric: func(tags map[string]string) Metric {
			return r.Rated(vec.rated).
				WithTags(tags).
				Counter(name).(*Counter)
		},
	}
	return &CounterVec{vec: vec}
}

// With creates new or returns existing counter with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *CounterVec) With(tags map[string]string) metrics.Counter {
	return v.vec.with(tags).(*Counter)
}

var _ metrics.GaugeVec = (*GaugeVec)(nil)

// GaugeVec stores gauges and
// implements metrics.GaugeVec interface
type GaugeVec struct {
	vec *metricsVector
}

// GaugeVec creates a new gauges vector with given metric name and
// partitioned by the given label names.
func (r *Registry) GaugeVec(name string, labels []string) metrics.GaugeVec {
	return &GaugeVec{
		vec: &metricsVector{
			labels: append([]string(nil), labels...),
			newMetric: func(tags map[string]string) Metric {
				return r.WithTags(tags).Gauge(name).(*Gauge)
			},
		},
	}
}

// With creates new or returns existing gauge with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *GaugeVec) With(tags map[string]string) metrics.Gauge {
	return v.vec.with(tags).(*Gauge)
}

var _ metrics.TimerVec = (*TimerVec)(nil)

// TimerVec stores timers and
// implements metrics.TimerVec interface
type TimerVec struct {
	vec *metricsVector
}

// TimerVec creates a new timers vector with given metric name and
// partitioned by the given label names.
func (r *Registry) TimerVec(name string, labels []string) metrics.TimerVec {
	return &TimerVec{
		vec: &metricsVector{
			labels: append([]string(nil), labels...),
			newMetric: func(tags map[string]string) Metric {
				return r.WithTags(tags).Timer(name).(*Timer)
			},
		},
	}
}

// With creates new or returns existing timer with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *TimerVec) With(tags map[string]string) metrics.Timer {
	return v.vec.with(tags).(*Timer)
}

var _ metrics.HistogramVec = (*HistogramVec)(nil)

// HistogramVec stores histograms and
// implements metrics.HistogramVec interface
type HistogramVec struct {
	vec *metricsVector
}

// HistogramVec creates a new histograms vector with given metric name and buckets and
// partitioned by the given label names.
func (r *Registry) HistogramVec(name string, buckets metrics.Buckets, labels []string) metrics.HistogramVec {
	var vec *metricsVector
	vec = &metricsVector{
		labels: append([]string(nil), labels...),
		rated:  r.rated,
		newMetric: func(tags map[string]string) Metric {
			return r.Rated(vec.rated).
				WithTags(tags).
				Histogram(name, buckets).(*Histogram)
		},
	}
	return &HistogramVec{vec: vec}
}

// With creates new or returns existing histogram with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *HistogramVec) With(tags map[string]string) metrics.Histogram {
	return v.vec.with(tags).(*Histogram)
}

var _ metrics.TimerVec = (*DurationHistogramVec)(nil)

// DurationHistogramVec stores duration histograms and
// implements metrics.TimerVec interface
type DurationHistogramVec struct {
	vec *metricsVector
}

// DurationHistogramVec creates a new duration histograms vector with given metric name and buckets and
// partitioned by the given label names.
func (r *Registry) DurationHistogramVec(name string, buckets metrics.DurationBuckets, labels []string) metrics.TimerVec {
	var vec *metricsVector
	vec = &metricsVector{
		labels: append([]string(nil), labels...),
		rated:  r.rated,
		newMetric: func(tags map[string]string) Metric {
			return r.Rated(vec.rated).
				WithTags(tags).
				DurationHistogram(name, buckets).(*Histogram)
		},
	}
	return &DurationHistogramVec{vec: vec}
}

// With creates new or returns existing duration histogram with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *DurationHistogramVec) With(tags map[string]string) metrics.Timer {
	return v.vec.with(tags).(*Histogram)
}
