package server

import (
	"context"
	"time"

	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/mlock/internal/mlockdb"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/models"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	timeToSleep = 10 * time.Second
)

type Stats struct {
	MlockDB mlockdb.MlockDB
	L       log.Logger
}

func (s *Stats) Run(ctx context.Context) {
	s.L.Debug("start stats thread")
	metric := prometheus.NewGauge(prometheus.GaugeOpts{Name: "mlock_max_locks_stale_time"})
	if err := prometheus.Register(metric); err != nil {
		s.L.Fatal("can not register metric", log.Error(err))
	}
	for {
		locks, err := s.listLocks(ctx)
		if err != nil {
			s.L.Error("can not list locks", log.Error(err))
			time.Sleep(timeToSleep)
			continue
		}

		var staleTime time.Duration
		if len(locks) > 0 {
			oldestTime := oldestLockTime(locks)
			staleTime = time.Since(oldestTime)
		}

		metric.Set(staleTime.Truncate(time.Second).Seconds())
		s.L.Debugf("sleep %s", timeToSleep.String())
		time.Sleep(timeToSleep)
	}
}

func (s *Stats) listLocks(ctx context.Context) ([]models.Lock, error) {
	var (
		res     []models.Lock
		hasMore = true
		locks   []models.Lock
		err     error
		offset  int64
	)

	for hasMore {
		locks, hasMore, err = s.MlockDB.GetLocks(ctx, "", defaultLimit, offset)
		if err != nil {
			return nil, err
		}

		res = append(res, locks...)
		offset += defaultLimit
	}

	return res, nil
}

func oldestLockTime(locks []models.Lock) time.Time {
	var res time.Time
	for _, lock := range locks {
		if res.IsZero() || lock.CreateTS.Before(res) {
			res = lock.CreateTS
		}
	}
	return res
}
