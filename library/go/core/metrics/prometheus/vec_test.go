package prometheus

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/metrics"
)

func TestCounterVec(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())
	vec := rg.CounterVec("ololo", []string{"shimba", "looken"})
	mt := vec.With(map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	})

	assert.IsType(t, &CounterVec{}, vec)
	assert.IsType(t, &Counter{}, mt)
}

func TestGaugeVec(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())
	vec := rg.GaugeVec("ololo", []string{"shimba", "looken"})
	mt := vec.With(map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	})

	assert.IsType(t, &GaugeVec{}, vec)
	assert.IsType(t, &Gauge{}, mt)
}

func TestTimerVec(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())
	vec := rg.TimerVec("ololo", []string{"shimba", "looken"})
	mt := vec.With(map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	})

	assert.IsType(t, &TimerVec{}, vec)
	assert.IsType(t, &Timer{}, mt)
}

func TestHistogramVec(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())
	buckets := metrics.NewBuckets(1, 2, 3)
	vec := rg.HistogramVec("ololo", buckets, []string{"shimba", "looken"})
	mt := vec.With(map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	})

	assert.IsType(t, &HistogramVec{}, vec)
	assert.IsType(t, &Histogram{}, mt)
}

func TestDurationHistogramVec(t *testing.T) {
	rg := NewRegistry(NewRegistryOpts())
	buckets := metrics.NewDurationBuckets(1, 2, 3)
	vec := rg.DurationHistogramVec("ololo", buckets, []string{"shimba", "looken"})
	mt := vec.With(map[string]string{
		"shimba": "boomba",
		"looken": "tooken",
	})

	assert.IsType(t, &DurationHistogramVec{}, vec)
	assert.IsType(t, &Histogram{}, mt)
}
