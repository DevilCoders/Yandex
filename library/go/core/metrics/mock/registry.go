package mock

import (
	"sync"

	"go.uber.org/atomic"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/internal/pkg/metricsutil"
	"a.yandex-team.ru/library/go/core/metrics/internal/pkg/registryutil"
)

var _ metrics.Registry = (*Registry)(nil)

type Registry struct {
	separator                  string
	prefix                     string
	tags                       map[string]string
	allowLoadRegisteredMetrics bool

	subregistries map[string]*Registry
	m             *sync.Mutex

	metrics *sync.Map
}

func NewRegistry(opts *RegistryOpts) *Registry {
	r := &Registry{
		separator: ".",

		subregistries: make(map[string]*Registry),
		m:             new(sync.Mutex),

		metrics: new(sync.Map),
	}

	if opts != nil {
		r.separator = string(opts.Separator)
		r.prefix = opts.Prefix
		r.tags = opts.Tags
		r.allowLoadRegisteredMetrics = opts.AllowLoadRegisteredMetrics
	}

	return r
}

// WithTags creates new sub-scope, where each metric has tags attached to it.
func (r Registry) WithTags(tags map[string]string) metrics.Registry {
	return r.newSubregistry(r.prefix, registryutil.MergeTags(r.tags, tags))
}

// WithPrefix creates new sub-scope, where each metric has prefix added to it name.
func (r Registry) WithPrefix(prefix string) metrics.Registry {
	return r.newSubregistry(registryutil.BuildFQName(r.separator, r.prefix, prefix), r.tags)
}

func (r Registry) ComposeName(parts ...string) string {
	return registryutil.BuildFQName(r.separator, parts...)
}

func (r Registry) Counter(name string) metrics.Counter {
	s := &Counter{
		Name:  r.newMetricName(name),
		Tags:  r.tags,
		Value: new(atomic.Int64),
	}

	key := registryutil.BuildRegistryKey(s.Name, r.tags)
	if val, loaded := r.metrics.LoadOrStore(key, s); loaded {
		if r.allowLoadRegisteredMetrics {
			return val.(*Counter)
		}
		panic("metric with key " + key + " already registered")
	}
	return s
}

func (r Registry) FuncCounter(name string, _ func() int64) {
	metricName := r.newMetricName(name)
	key := registryutil.BuildRegistryKey(metricName, r.tags)
	if _, loaded := r.metrics.LoadOrStore(key, struct{}{}); loaded {
		panic("metric with key " + key + " already registered")
	}
}

func (r Registry) Gauge(name string) metrics.Gauge {
	s := &Gauge{
		Name:  r.newMetricName(name),
		Tags:  r.tags,
		Value: new(atomic.Float64),
	}

	key := registryutil.BuildRegistryKey(s.Name, r.tags)
	if val, loaded := r.metrics.LoadOrStore(key, s); loaded {
		if r.allowLoadRegisteredMetrics {
			return val.(*Gauge)
		}
		panic("metric with key " + key + " already registered")
	}
	return s
}

func (r Registry) FuncGauge(name string, _ func() float64) {
	metricName := r.newMetricName(name)
	key := registryutil.BuildRegistryKey(metricName, r.tags)
	if _, loaded := r.metrics.LoadOrStore(key, struct{}{}); loaded {
		panic("metric with key " + key + " already registered")
	}
}

func (r Registry) Timer(name string) metrics.Timer {
	s := &Timer{
		Name:  r.newMetricName(name),
		Tags:  r.tags,
		Value: new(atomic.Duration),
	}

	key := registryutil.BuildRegistryKey(s.Name, r.tags)
	if val, loaded := r.metrics.LoadOrStore(key, s); loaded {
		if r.allowLoadRegisteredMetrics {
			return val.(*Timer)
		}
		panic("metric with key " + key + " already registered")
	}
	return s
}

func (r Registry) Histogram(name string, buckets metrics.Buckets) metrics.Histogram {
	s := &Histogram{
		Name:         r.newMetricName(name),
		Tags:         r.tags,
		BucketBounds: metricsutil.BucketsBounds(buckets),
		BucketValues: make([]int64, buckets.Size()),
		InfValue:     new(atomic.Int64),
	}

	key := registryutil.BuildRegistryKey(s.Name, r.tags)
	if val, loaded := r.metrics.LoadOrStore(key, s); loaded {
		if r.allowLoadRegisteredMetrics {
			return val.(*Histogram)
		}
		panic("metric with key " + key + " already registered")
	}
	return s
}

func (r Registry) DurationHistogram(name string, buckets metrics.DurationBuckets) metrics.Timer {
	s := &Histogram{
		Name:         r.newMetricName(name),
		Tags:         r.tags,
		BucketBounds: metricsutil.DurationBucketsBounds(buckets),
		BucketValues: make([]int64, buckets.Size()),
		InfValue:     new(atomic.Int64),
	}

	key := registryutil.BuildRegistryKey(s.Name, r.tags)
	if val, loaded := r.metrics.LoadOrStore(key, s); loaded {
		if r.allowLoadRegisteredMetrics {
			return val.(*Histogram)
		}
		panic("metric with key " + key + " already registered")
	}
	return s
}

func (r *Registry) newSubregistry(prefix string, tags map[string]string) *Registry {
	registryKey := registryutil.BuildRegistryKey(prefix, tags)

	r.m.Lock()
	defer r.m.Unlock()

	if existing, ok := r.subregistries[registryKey]; ok {
		return existing
	}

	subregistry := &Registry{
		separator:                  r.separator,
		prefix:                     prefix,
		tags:                       tags,
		allowLoadRegisteredMetrics: r.allowLoadRegisteredMetrics,

		subregistries: r.subregistries,
		m:             r.m,

		metrics: r.metrics,
	}

	r.subregistries[registryKey] = subregistry
	return subregistry
}

func (r *Registry) newMetricName(name string) string {
	return registryutil.BuildFQName(r.separator, r.prefix, name)
}
