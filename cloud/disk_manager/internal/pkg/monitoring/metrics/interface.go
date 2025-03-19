package metrics

import (
	"context"
	"time"

	"a.yandex-team.ru/library/go/core/metrics"
)

////////////////////////////////////////////////////////////////////////////////

type Registry interface {
	metrics.Registry
}

type Counter interface {
	metrics.Counter
}

type CounterVec interface {
	metrics.CounterVec
}

type Gauge interface {
	metrics.Gauge
}

type GaugeVec interface {
	metrics.GaugeVec
}

type Histogram interface {
	metrics.Histogram
}

type Timer interface {
	metrics.Timer
}

type TimerVec interface {
	metrics.TimerVec
}

type Buckets interface {
	metrics.Buckets
}

type DurationBuckets interface {
	metrics.DurationBuckets
}

type DelayedTimer struct {
	startTime time.Time
	timer     Timer
}

func StartDelayedTimer(timer Timer) DelayedTimer {
	return DelayedTimer{
		startTime: time.Now(),
		timer:     timer,
	}
}

func (t *DelayedTimer) Stop() {
	t.timer.RecordDuration(time.Since(t.startTime))
}

////////////////////////////////////////////////////////////////////////////////

type tagsKey struct{}

func mergeTags(
	left map[string]string,
	right map[string]string,
) map[string]string {

	if len(left) == 0 {
		return right
	}

	if len(right) == 0 {
		return left
	}

	tags := make(map[string]string)

	for k, v := range left {
		tags[k] = v
	}

	for k, v := range right {
		tags[k] = v
	}

	return tags
}

func WithTags(
	ctx context.Context,
	tags map[string]string,
) context.Context {

	existingTags, _ := ctx.Value(tagsKey{}).(map[string]string)
	return context.WithValue(ctx, tagsKey{}, mergeTags(existingTags, tags))
}
