package mock

import (
	"go.uber.org/atomic"

	"a.yandex-team.ru/library/go/core/metrics"
)

var _ metrics.Gauge = (*Gauge)(nil)

// Gauge tracks single float64 value.
type Gauge struct {
	Name  string
	Tags  map[string]string
	Value *atomic.Float64
}

func (g *Gauge) Set(value float64) {
	g.Value.Store(value)
}

func (g *Gauge) Add(value float64) {
	g.Value.Add(value)
}
