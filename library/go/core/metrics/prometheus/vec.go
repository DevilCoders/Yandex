package prometheus

import (
	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/internal/pkg/metricsutil"
)

var _ metrics.CounterVec = (*CounterVec)(nil)

// CounterVec wraps prometheus.CounterVec
// and implements metrics.CounterVec interface
type CounterVec struct {
	vec *prometheus.CounterVec
}

// CounterVec creates a new counters vector with given metric name and
// partitioned by the given label names.
func (r *Registry) CounterVec(name string, labels []string) metrics.CounterVec {
	vec := prometheus.NewCounterVec(prometheus.CounterOpts{
		Namespace:   r.prefix,
		Name:        name,
		ConstLabels: r.tags,
	}, labels)

	r.rg.MustRegister(vec)
	return &CounterVec{vec: vec}
}

// With creates new or returns existing counter with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *CounterVec) With(tags map[string]string) metrics.Counter {
	return &Counter{cnt: v.vec.With(tags)}
}

var _ metrics.GaugeVec = (*GaugeVec)(nil)

// GaugeVec wraps prometheus.GaugeVec
// and implements metrics.GaugeVec interface
type GaugeVec struct {
	vec *prometheus.GaugeVec
}

// GaugeVec creates a new gauges vector with given metric name and
// partitioned by the given label names.
func (r *Registry) GaugeVec(name string, labels []string) metrics.GaugeVec {
	vec := prometheus.NewGaugeVec(prometheus.GaugeOpts{
		Namespace:   r.prefix,
		Name:        name,
		ConstLabels: r.tags,
	}, labels)

	r.rg.MustRegister(vec)
	return &GaugeVec{vec: vec}
}

// With creates new or returns existing gauge with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *GaugeVec) With(tags map[string]string) metrics.Gauge {
	return &Gauge{gg: v.vec.With(tags)}
}

var _ metrics.TimerVec = (*TimerVec)(nil)

// TimerVec wraps prometheus.GaugeVec
// and implements metrics.TimerVec interface
type TimerVec struct {
	vec *prometheus.GaugeVec
}

// TimerVec creates a new timers vector with given metric name and
// partitioned by the given label names.
func (r *Registry) TimerVec(name string, labels []string) metrics.TimerVec {
	vec := prometheus.NewGaugeVec(prometheus.GaugeOpts{
		Namespace:   r.prefix,
		Name:        name,
		ConstLabels: r.tags,
	}, labels)

	r.rg.MustRegister(vec)
	return &TimerVec{vec: vec}
}

// With creates new or returns existing timer with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *TimerVec) With(tags map[string]string) metrics.Timer {
	return &Timer{gg: v.vec.With(tags)}
}

var _ metrics.HistogramVec = (*HistogramVec)(nil)

// HistogramVec wraps prometheus.HistogramVec
// and implements metrics.HistogramVec interface
type HistogramVec struct {
	vec *prometheus.HistogramVec
}

// HistogramVec creates a new histograms vector with given metric name and buckets and
// partitioned by the given label names.
func (r *Registry) HistogramVec(name string, buckets metrics.Buckets, labels []string) metrics.HistogramVec {
	vec := prometheus.NewHistogramVec(prometheus.HistogramOpts{
		Namespace:   r.prefix,
		Name:        name,
		ConstLabels: r.tags,
		Buckets:     metricsutil.BucketsBounds(buckets),
	}, labels)

	r.rg.MustRegister(vec)
	return &HistogramVec{vec: vec}
}

// With creates new or returns existing histogram with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *HistogramVec) With(tags map[string]string) metrics.Histogram {
	return &Histogram{hm: v.vec.With(tags)}
}

var _ metrics.TimerVec = (*DurationHistogramVec)(nil)

// DurationHistogramVec wraps prometheus.HistogramVec
// and implements metrics.TimerVec interface
type DurationHistogramVec struct {
	vec *prometheus.HistogramVec
}

// DurationHistogramVec creates a new duration histograms vector with given metric name and buckets and
// partitioned by the given label names.
func (r *Registry) DurationHistogramVec(name string, buckets metrics.DurationBuckets, labels []string) metrics.TimerVec {
	vec := prometheus.NewHistogramVec(prometheus.HistogramOpts{
		Namespace:   r.prefix,
		Name:        name,
		ConstLabels: r.tags,
		Buckets:     metricsutil.DurationBucketsBounds(buckets),
	}, labels)

	r.rg.MustRegister(vec)
	return &DurationHistogramVec{vec: vec}
}

// With creates new or returns existing duration histogram with given tags from vector.
// It will panic if tags keys set is not equal to vector labels.
func (v *DurationHistogramVec) With(tags map[string]string) metrics.Timer {
	return &Histogram{hm: v.vec.With(tags)}
}
