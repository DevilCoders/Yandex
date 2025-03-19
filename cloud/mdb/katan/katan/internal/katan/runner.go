package katan

import (
	"context"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/library/go/core/log"
)

type RolloutRunnerConfig struct {
	PendingInterval time.Duration
	Parallel        int32
	MaxRollouts     int32
}

func RunRollouts(ctx context.Context, pendingHandle func() (katandb.Rollout, error), startHandle func(katandb.Rollout), cfg RolloutRunnerConfig, L log.Logger) {
	var started, finished int32

	actualFinished := func() int32 {
		return atomic.LoadInt32(&finished)
	}

	timer := time.NewTimer(0)
	for {
		select {
		case <-ctx.Done():
			L.Infof("context is done. There are %d started and %d finished rollouts", started, actualFinished())
			return
		case <-timer.C:
			L.Debugf("wakeup by timer started: %d, finished: %d", started, actualFinished())
			for {
				if (started - actualFinished()) >= cfg.Parallel {
					// more than parallel limit is running -> go to sleep
					break
				}

				if cfg.MaxRollouts > 0 && started >= cfg.MaxRollouts {
					// we start more then MaxRollouts
					break
				}

				pending, err := pendingHandle()
				if err != nil {
					if err != katandb.ErrNoDataFound {
						L.Warnf("fail to get PendingRollouts: %s", err)
					}
					break
				}

				L.Infof("starting rollout %+v. There are %d started %d finished", pending, started, actualFinished())
				started++
				go func() {
					defer func() { atomic.AddInt32(&finished, 1); L.Debug("startHandle done") }()
					startHandle(pending)
				}()
			}

			if cfg.MaxRollouts > 0 && started >= cfg.MaxRollouts && started == actualFinished() {
				L.Infof("All rollouts from MaxRollouts: %d are finished", cfg.MaxRollouts)
				return
			}
			timer.Reset(cfg.PendingInterval)
		}
	}
}
