package nop

import "a.yandex-team.ru/library/go/core/metrics"

var _ metrics.Counter = (*Counter)(nil)

type Counter struct{}

func (Counter) Inc() {}

func (Counter) Add(_ int64) {}

var _ metrics.CounterVec = (*CounterVec)(nil)

type CounterVec struct{}

func (t CounterVec) With(_ map[string]string) metrics.Counter {
	return Counter{}
}
