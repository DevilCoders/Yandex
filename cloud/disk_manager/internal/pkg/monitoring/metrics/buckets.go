package metrics

import (
	"time"

	"a.yandex-team.ru/library/go/core/metrics"
)

////////////////////////////////////////////////////////////////////////////////

func MakeLinearBuckets(start float64, width float64, n int) Buckets {
	return metrics.MakeLinearBuckets(start, width, n)
}

func MakeExponentialBuckets(start float64, factor float64, n int) Buckets {
	return metrics.MakeExponentialBuckets(start, factor, n)
}

func NewDurationBuckets(bk ...time.Duration) DurationBuckets {
	return metrics.NewDurationBuckets(bk...)
}
