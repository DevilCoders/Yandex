package roller

import (
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// RollClusters takes clusters and run handle on them
func RollClusters(clusters []models.Cluster, roller func(models.Cluster) error, accelerate Accelerator, L log.Logger) error {
	var rollErrors []error
	rollReturns := make(chan error)

	var runningAt, doneAt int
	var skippedCount int

	for iteration := 0; ; iteration++ {
		stats := RolloutStat{
			Done:    doneAt,
			Running: runningAt - doneAt,
			Skipped: skippedCount,
			Errors:  len(rollErrors),
		}
		L.Debugf("rollout stats are %+v. Iteration is %d", stats, iteration)
		if runningAt < len(clusters) && accelerate(stats) {
			L.Debugf("run new roller for %+v", clusters[runningAt])
			go func(cluster models.Cluster) {
				rollReturns <- roller(cluster)
			}(clusters[runningAt])
			runningAt++
			continue
		}

		if doneAt == runningAt {
			L.Debug("no more running rollouts")
			break
		}

		rollErr := <-rollReturns
		doneAt++
		if rollErr != nil {
			if xerrors.Is(rollErr, cluster.ErrRolloutSkipped) || xerrors.Is(rollErr, cluster.ErrRolloutCanceled) {
				skippedCount++
				continue
			}
			L.Warnf("rollout failed: %s. Don't start new rollouts. Currently %d rollouts running", rollErr, runningAt-doneAt)
			rollErrors = append(rollErrors, rollErr)
		}
	}

	close(rollReturns)

	for _, rollErr := range rollErrors {
		return rollErr
	}
	return nil
}
