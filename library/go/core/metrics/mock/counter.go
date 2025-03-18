package mock

import (
	"go.uber.org/atomic"

	"a.yandex-team.ru/library/go/core/metrics"
)

var _ metrics.Counter = (*Counter)(nil)

// Counter tracks monotonically increasing value.
type Counter struct {
	Name  string
	Tags  map[string]string
	Value *atomic.Int64
}

// Inc increments counter by 1.
func (c *Counter) Inc() {
	c.Add(1)
}

// Add adds delta to the counter. Delta must be >=0.
func (c *Counter) Add(delta int64) {
	c.Value.Add(delta)
}
