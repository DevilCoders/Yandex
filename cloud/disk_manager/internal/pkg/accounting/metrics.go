package accounting

import (
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
)

////////////////////////////////////////////////////////////////////////////////

type accountingMetrics struct {
	// tasks accounting
	tasksCreateCount metrics.CounterVec

	// snapshots accounting
	snapshotsReadBytes  metrics.CounterVec
	snapshotsWriteBytes metrics.CounterVec
}

////////////////////////////////////////////////////////////////////////////////

var metricsInstance *accountingMetrics

func getMetrics() *accountingMetrics {
	return metricsInstance
}

func initMetrics(registry metrics.Registry) {
	metricsInstance = &accountingMetrics{
		tasksCreateCount: registry.CounterVec("tasks/CreateCount", []string{
			"taskType",
			"cloudID",
			"folderID",
		}),

		snapshotsReadBytes: registry.CounterVec("snapshots/ReadBytes", []string{
			"snapshotID",
		}),
		snapshotsWriteBytes: registry.CounterVec("snapshots/WriteBytes", []string{
			"snapshotID",
		}),
	}
}
