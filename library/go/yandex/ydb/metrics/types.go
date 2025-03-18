package metrics

import (
	"strings"
	"sync"
	"time"

	"github.com/ydb-platform/ydb-go-sdk-metrics/registry"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	coreMetrics "a.yandex-team.ru/library/go/core/metrics"
)

const (
	defaultNamespace = "ydb_go_sdk"
	defaultSeparator = "/"
)

var (
	defaultTimerBuckets = coreMetrics.MakeExponentialDurationBuckets(time.Millisecond, 1.25, 50)
)

type buckets []float64

func (b buckets) Size() int {
	return len(b)
}

func (b buckets) MapValue(v float64) (idx int) {
	for _, bound := range b {
		if v < bound {
			break
		}
		idx++
	}
	return
}

func (b buckets) UpperBound(idx int) float64 {
	if idx > len(b)-1 {
		panic("idx is out of bounds")
	}
	return b[idx]
}

type config struct {
	details   trace.Details
	separator string
	registry  coreMetrics.Registry
	namespace string

	countersMtx   sync.Mutex
	counters      map[string]*counterVec
	gaugesMtx     sync.Mutex
	gauges        map[string]*gaugeVec
	timersMtx     sync.Mutex
	timers        map[string]*timerVec
	histogramsMtx sync.Mutex
	histograms    map[string]*histogramVec
}

func (c *config) CounterVec(name string, labelNames ...string) registry.CounterVec {
	name = c.join(c.namespace, name)
	c.countersMtx.Lock()
	defer c.countersMtx.Unlock()
	if cnt, ok := c.counters[name]; ok {
		return cnt
	}
	cnt := &counterVec{c: c.registry.CounterVec(name, append([]string{}, labelNames...))}
	c.counters[name] = cnt
	return cnt
}

func (c *config) HistogramVec(name string, bb []float64, labelNames ...string) registry.HistogramVec {
	name = c.join(c.namespace, name)
	c.histogramsMtx.Lock()
	defer c.histogramsMtx.Unlock()
	if h, ok := c.histograms[name]; ok {
		return h
	}
	h := &histogramVec{h: c.registry.HistogramVec(name, buckets(bb), append([]string{}, labelNames...))}
	c.histograms[name] = h
	return h
}

func (c *config) TimerVec(name string, labelNames ...string) registry.TimerVec {
	name = c.join(c.namespace, name)
	c.timersMtx.Lock()
	defer c.timersMtx.Unlock()
	if t, ok := c.timers[name]; ok {
		return t
	}
	t := &timerVec{t: c.registry.DurationHistogramVec(name, defaultTimerBuckets, append([]string{}, labelNames...))}
	c.timers[name] = t
	return t
}

func (c *config) join(a, b string) string {
	if a == "" {
		return b
	}
	if b == "" {
		return ""
	}
	return strings.Join([]string{a, b}, c.separator)
}

func (c *config) WithSystem(subsystem string) registry.Config {
	return &config{
		separator:  c.separator,
		details:    c.details,
		registry:   c.registry,
		namespace:  c.join(c.namespace, subsystem),
		counters:   make(map[string]*counterVec),
		gauges:     make(map[string]*gaugeVec),
		timers:     make(map[string]*timerVec),
		histograms: make(map[string]*histogramVec),
	}
}

func (c *config) Details() trace.Details {
	return c.details
}

type counterVec struct {
	c coreMetrics.CounterVec
}

func (c *counterVec) With(kv map[string]string) registry.Counter {
	cnt := c.c.With(kv)
	return cnt
}

type gaugeVec struct {
	g coreMetrics.GaugeVec
}

type histogramVec struct {
	h coreMetrics.HistogramVec
}

type timerVec struct {
	t coreMetrics.TimerVec
}

func (tv *timerVec) With(kv map[string]string) registry.Timer {
	t := tv.t.With(kv)
	return &timer{t: t}
}

type histogram struct {
	h coreMetrics.Histogram
}

type timer struct {
	t coreMetrics.Timer
}

func (t *timer) Record(d time.Duration) {
	t.t.RecordDuration(d)
}

func (h *histogram) Record(v float64) {
	h.h.RecordValue(v)
}

func (hv *histogramVec) With(kv map[string]string) registry.Histogram {
	h := hv.h.With(kv)
	return &histogram{h: h}
}

func (gv *gaugeVec) With(kv map[string]string) registry.Gauge {
	gauge := gv.g.With(kv)
	return gauge
}

func (c *config) GaugeVec(name string, labelNames ...string) registry.GaugeVec {
	name = c.join(c.namespace, name)
	c.gaugesMtx.Lock()
	defer c.gaugesMtx.Unlock()
	if g, ok := c.gauges[name]; ok {
		return g
	}
	g := &gaugeVec{g: c.registry.GaugeVec(name, append([]string{}, labelNames...))}
	c.gauges[name] = g
	return g
}

type option func(*config)

func WithNamespace(namespace string) option {
	return func(c *config) {
		c.namespace = namespace
	}
}

func WithDetails(details trace.Details) option {
	return func(c *config) {
		c.details |= details
	}
}

func WithSeparator(separator string) option {
	return func(c *config) {
		c.separator = separator
	}
}
