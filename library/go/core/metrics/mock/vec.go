package mock

import (
	"sync"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/internal/pkg/registryutil"
)

type MetricsVector interface {
	With(map[string]string) interface{}
}

// Vector is base implementation of vector of metrics of any supported type
type Vector struct {
	Labels    []string
	Metrics   sync.Map
	NewMetric func(map[string]string) interface{}
}

// VecMetric is vector metric with initializer
type VecMetric struct {
	init  sync.Once
	value interface{}
}

func (v *Vector) With(tags map[string]string) interface{} {
	hv, err := registryutil.VectorHash(tags, v.Labels)
	if err != nil {
		panic(err)
	}

	val, _ := v.Metrics.LoadOrStore(hv, new(VecMetric))

	vs := val.(*VecMetric)
	vs.init.Do(func() {
		vs.value = v.NewMetric(tags)
	})

	return vs.value
}

var _ metrics.CounterVec = (*CounterVec)(nil)

// CounterVec stores counters and
// implements metrics.CounterVec interface
type CounterVec struct {
	Vec MetricsVector
}

// CounterVec creates a new counters vector with given metric name and
// partitioned by the given label names.
func (r *Registry) CounterVec(name string, labels []string) metrics.CounterVec {
	return &CounterVec{
		Vec: &Vector{
			Labels: append([]string(nil), labels...),
			NewMetric: func(tags map[string]string) interface{} {
				return r.WithTags(tags).Counter(name)
			},
		},
	}
}

// With creates new or returns existing counter with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *CounterVec) With(tags map[string]string) metrics.Counter {
	return v.Vec.With(tags).(*Counter)
}

var _ metrics.GaugeVec = new(GaugeVec)

// GaugeVec stores gauges and
// implements metrics.GaugeVec interface
type GaugeVec struct {
	Vec MetricsVector
}

// GaugeVec creates a new gauges vector with given metric name and
// partitioned by the given label names.
func (r *Registry) GaugeVec(name string, labels []string) metrics.GaugeVec {
	return &GaugeVec{
		Vec: &Vector{
			Labels: append([]string(nil), labels...),
			NewMetric: func(tags map[string]string) interface{} {
				return r.WithTags(tags).Gauge(name)
			},
		},
	}
}

// With creates new or returns existing gauge with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *GaugeVec) With(tags map[string]string) metrics.Gauge {
	return v.Vec.With(tags).(*Gauge)
}

var _ metrics.TimerVec = new(TimerVec)

// TimerVec stores timers and
// implements metrics.TimerVec interface
type TimerVec struct {
	Vec MetricsVector
}

// TimerVec creates a new timers vector with given metric name and
// partitioned by the given label names.
func (r *Registry) TimerVec(name string, labels []string) metrics.TimerVec {
	return &TimerVec{
		Vec: &Vector{
			Labels: append([]string(nil), labels...),
			NewMetric: func(tags map[string]string) interface{} {
				return r.WithTags(tags).Timer(name)
			},
		},
	}
}

// With creates new or returns existing timer with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *TimerVec) With(tags map[string]string) metrics.Timer {
	return v.Vec.With(tags).(*Timer)
}

var _ metrics.HistogramVec = (*HistogramVec)(nil)

// HistogramVec stores histograms and
// implements metrics.HistogramVec interface
type HistogramVec struct {
	Vec MetricsVector
}

// HistogramVec creates a new histograms vector with given metric name and buckets and
// partitioned by the given label names.
func (r *Registry) HistogramVec(name string, buckets metrics.Buckets, labels []string) metrics.HistogramVec {
	return &HistogramVec{
		Vec: &Vector{
			Labels: append([]string(nil), labels...),
			NewMetric: func(tags map[string]string) interface{} {
				return r.WithTags(tags).Histogram(name, buckets)
			},
		},
	}
}

// With creates new or returns existing histogram with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *HistogramVec) With(tags map[string]string) metrics.Histogram {
	return v.Vec.With(tags).(*Histogram)
}

var _ metrics.TimerVec = (*DurationHistogramVec)(nil)

// DurationHistogramVec stores duration histograms and
// implements metrics.TimerVec interface
type DurationHistogramVec struct {
	Vec MetricsVector
}

// DurationHistogramVec creates a new duration histograms vector with given metric name and buckets and
// partitioned by the given label names.
func (r *Registry) DurationHistogramVec(name string, buckets metrics.DurationBuckets, labels []string) metrics.TimerVec {
	return &DurationHistogramVec{
		Vec: &Vector{
			Labels: append([]string(nil), labels...),
			NewMetric: func(tags map[string]string) interface{} {
				return r.WithTags(tags).DurationHistogram(name, buckets)
			},
		},
	}
}

// With creates new or returns existing duration histogram with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *DurationHistogramVec) With(tags map[string]string) metrics.Timer {
	return v.Vec.With(tags).(*Histogram)
}
