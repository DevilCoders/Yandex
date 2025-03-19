package stat

import (
	"a.yandex-team.ru/library/go/core/metrics"
)

type MaintenanceStat struct {
	ConfigCount                metrics.Counter
	ConfigErrorCount           metrics.Counter
	MaintenanceDuration        metrics.Timer
	ConfigPlanClusterCount     metrics.CounterVec
	ConfigSelectedClusterCount metrics.CounterVec
}
