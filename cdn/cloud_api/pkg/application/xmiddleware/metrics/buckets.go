package metrics

import (
	"time"

	"a.yandex-team.ru/library/go/core/metrics"
)

// DefaultDurationBuckets provides 35 buckets from 250us to 242s
var DefaultDurationBuckets = metrics.MakeExponentialDurationBuckets(250*time.Microsecond, 1.5, 35)
