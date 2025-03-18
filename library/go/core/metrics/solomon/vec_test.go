package solomon

import (
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/metrics"
)

func testSyncMapLen(m *sync.Map) int {
	var length int
	m.Range(func(_, _ interface{}) bool {
		length++
		return true
	})
	return length
}

func TestVec(t *testing.T) {
	type args struct {
		name     string
		labels   []string
		buckets  metrics.Buckets
		dbuckets metrics.DurationBuckets
	}

	testCases := []struct {
		name         string
		args         args
		expectedType interface{}
		expectLabels []string
	}{
		{
			name: "CounterVec",
			args: args{
				name:   "cntvec",
				labels: []string{"shimba", "looken"},
			},
			expectedType: &CounterVec{},
			expectLabels: []string{"shimba", "looken"},
		},
		{
			name: "GaugeVec",
			args: args{
				name:   "ggvec",
				labels: []string{"shimba", "looken"},
			},
			expectedType: &GaugeVec{},
			expectLabels: []string{"shimba", "looken"},
		},
		{
			name: "TimerVec",
			args: args{
				name:   "tvec",
				labels: []string{"shimba", "looken"},
			},
			expectedType: &TimerVec{},
			expectLabels: []string{"shimba", "looken"},
		},
		{
			name: "HistogramVec",
			args: args{
				name:    "hvec",
				labels:  []string{"shimba", "looken"},
				buckets: metrics.NewBuckets(1, 2, 3, 4),
			},
			expectedType: &HistogramVec{},
			expectLabels: []string{"shimba", "looken"},
		},
		{
			name: "DurationHistogramVec",
			args: args{
				name:     "dhvec",
				labels:   []string{"shimba", "looken"},
				dbuckets: metrics.NewDurationBuckets(1, 2, 3, 4),
			},
			expectedType: &DurationHistogramVec{},
			expectLabels: []string{"shimba", "looken"},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			rg := NewRegistry(NewRegistryOpts())

			switch vect := tc.expectedType.(type) {
			case *CounterVec:
				vec := rg.CounterVec(tc.args.name, tc.args.labels)
				assert.IsType(t, vect, vec)
				assert.Equal(t, tc.expectLabels, vec.(*CounterVec).vec.labels)
			case *GaugeVec:
				vec := rg.GaugeVec(tc.args.name, tc.args.labels)
				assert.IsType(t, vect, vec)
				assert.Equal(t, tc.expectLabels, vec.(*GaugeVec).vec.labels)
			case *TimerVec:
				vec := rg.TimerVec(tc.args.name, tc.args.labels)
				assert.IsType(t, vect, vec)
				assert.Equal(t, tc.expectLabels, vec.(*TimerVec).vec.labels)
			case *HistogramVec:
				vec := rg.HistogramVec(tc.args.name, tc.args.buckets, tc.args.labels)
				assert.IsType(t, vect, vec)
				assert.Equal(t, tc.expectLabels, vec.(*HistogramVec).vec.labels)
			case *DurationHistogramVec:
				vec := rg.DurationHistogramVec(tc.args.name, tc.args.dbuckets, tc.args.labels)
				assert.IsType(t, vect, vec)
				assert.Equal(t, tc.expectLabels, vec.(*DurationHistogramVec).vec.labels)
			default:
				t.Errorf("unknown type: %T", vect)
			}
		})
	}
}

func TestCounterVecWith(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())

	t.Run("plain", func(t *testing.T) {
		vec := rg.CounterVec("ololo", []string{"shimba", "looken"})
		metric := vec.With(map[string]string{
			"shimba": "boomba",
			"looken": "tooken",
		})

		assert.IsType(t, &CounterVec{}, vec)
		assert.IsType(t, &Counter{}, metric)
		assert.Equal(t, typeCounter, metric.(*Counter).metricType)
	})

	t.Run("rated", func(t *testing.T) {
		vec := rg.CounterVec("ololo", []string{"shimba", "looken"})
		Rated(vec)
		metric := vec.With(map[string]string{
			"shimba": "boomba",
			"looken": "tooken",
		})

		assert.IsType(t, &CounterVec{}, vec)
		assert.IsType(t, &Counter{}, metric)
		assert.Equal(t, typeRated, metric.(*Counter).metricType)
	})
}

func TestGaugeVecWith(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())

	vec := rg.GaugeVec("ololo", []string{"shimba", "looken"})
	metric := vec.With(map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	})

	assert.IsType(t, &GaugeVec{}, vec)
	assert.IsType(t, &Gauge{}, metric)
	assert.Equal(t, typeGauge, metric.(*Gauge).metricType)
}

func TestTimerVecWith(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())
	vec := rg.TimerVec("ololo", []string{"shimba", "looken"})
	metric := vec.With(map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	})

	assert.IsType(t, &TimerVec{}, vec)
	assert.IsType(t, &Timer{}, metric)
	assert.Equal(t, typeGauge, metric.(*Timer).metricType)
}

func TestHistogramVecWith(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())

	t.Run("plain", func(t *testing.T) {
		buckets := metrics.NewBuckets(1, 2, 3)
		vec := rg.HistogramVec("ololo", buckets, []string{"shimba", "looken"})
		metric := vec.With(map[string]string{
			"shimba": "boomba",
			"looken": "tooken",
		})

		assert.IsType(t, &HistogramVec{}, vec)
		assert.IsType(t, &Histogram{}, metric)
		assert.Equal(t, typeHistogram, metric.(*Histogram).metricType)
	})

	t.Run("rated", func(t *testing.T) {
		buckets := metrics.NewBuckets(1, 2, 3)
		vec := rg.HistogramVec("ololo", buckets, []string{"shimba", "looken"})
		Rated(vec)
		metric := vec.With(map[string]string{
			"shimba": "boomba",
			"looken": "tooken",
		})

		assert.IsType(t, &HistogramVec{}, vec)
		assert.IsType(t, &Histogram{}, metric)
		assert.Equal(t, typeRatedHistogram, metric.(*Histogram).metricType)
	})
}

func TestDurationHistogramVecWith(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())

	t.Run("plain", func(t *testing.T) {
		buckets := metrics.NewDurationBuckets(1, 2, 3)
		vec := rg.DurationHistogramVec("ololo", buckets, []string{"shimba", "looken"})
		metric := vec.With(map[string]string{
			"shimba": "boomba",
			"looken": "tooken",
		})

		assert.IsType(t, &DurationHistogramVec{}, vec)
		assert.IsType(t, &Histogram{}, metric)
		assert.Equal(t, typeHistogram, metric.(*Histogram).metricType)
	})

	t.Run("rated", func(t *testing.T) {
		buckets := metrics.NewDurationBuckets(1, 2, 3)
		vec := rg.DurationHistogramVec("ololo", buckets, []string{"shimba", "looken"})
		Rated(vec)
		metric := vec.With(map[string]string{
			"shimba": "boomba",
			"looken": "tooken",
		})

		assert.IsType(t, &DurationHistogramVec{}, vec)
		assert.IsType(t, &Histogram{}, metric)
		assert.Equal(t, typeRatedHistogram, metric.(*Histogram).metricType)
	})
}

func TestMetricsVectorWith(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())

	name := "ololo"
	tags := map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	}

	vec := &metricsVector{
		labels: []string{"shimba", "looken"},
		newMetric: func(tags map[string]string) Metric {
			return rg.WithTags(tags).Counter(name).(*Counter)
		},
	}

	// check first counter
	metric := vec.with(tags)
	require.IsType(t, &Counter{}, metric)
	cnt := metric.(*Counter)
	assert.Equal(t, name, cnt.name)
	assert.Equal(t, tags, cnt.tags)

	// check vector length
	assert.Equal(t, 1, testSyncMapLen(&vec.metrics))

	// check same counter returned for same tags set
	cnt2 := vec.with(tags)
	assert.Same(t, cnt, cnt2)

	// check vector length
	assert.Equal(t, 1, testSyncMapLen(&vec.metrics))

	// return new counter
	cnt3 := vec.with(map[string]string{
		"shimba": "boomba",
		"looken": "cooken",
	})
	assert.NotSame(t, cnt, cnt3)

	// check vector length
	assert.Equal(t, 2, testSyncMapLen(&vec.metrics))

	// check for panic
	assert.Panics(t, func() {
		vec.with(map[string]string{"chicken": "cooken"})
	})
	assert.Panics(t, func() {
		vec.with(map[string]string{"shimba": "boomba", "chicken": "cooken"})
	})
}
