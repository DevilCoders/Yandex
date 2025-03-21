package nop

import (
	"time"

	"a.yandex-team.ru/library/go/core/metrics"
)

var _ metrics.Timer = (*Timer)(nil)

type Timer struct{}

func (Timer) RecordDuration(_ time.Duration) {}

var _ metrics.TimerVec = (*TimerVec)(nil)

type TimerVec struct{}

func (t TimerVec) With(_ map[string]string) metrics.Timer {
	return Timer{}
}
