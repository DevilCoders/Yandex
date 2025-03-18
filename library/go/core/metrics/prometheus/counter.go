package prometheus

import (
	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/library/go/core/metrics"
)

var _ metrics.Counter = (*Counter)(nil)

// Counter tracks monotonically increasing value.
type Counter struct {
	cnt prometheus.Counter
}

// Inc increments counter by 1.
func (c Counter) Inc() {
	c.cnt.Inc()
}

// Add adds delta to the counter. Delta must be >=0.
func (c Counter) Add(delta int64) {
	c.cnt.Add(float64(delta))
}
