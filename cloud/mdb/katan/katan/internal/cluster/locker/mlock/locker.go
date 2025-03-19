package mlock

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/locker"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const holder = "katan"

type Config struct {
	Disabled      bool                  `json:"disabled" yaml:"disabled"`
	Retries       retry.Config          `json:"retries" yaml:"retries"`
	CreateTimeout encodingutil.Duration `json:"create_timeout" yaml:"create_timeout"`
	CheckAttempts int                   `json:"check_attempts" yaml:"check_attempts"`
	CheckInterval encodingutil.Duration `json:"check_interval" yaml:"check_interval"`
}

func DefaultConfig() Config {
	config := Config{
		CreateTimeout: encodingutil.FromDuration(time.Second * 30),
		CheckAttempts: 18, // * CheckInterval (10sec) => 3 minutes
		CheckInterval: encodingutil.FromDuration(time.Second * 10),
		Retries:       retry.DefaultConfig(),
	}
	config.Retries.MaxRetries = 32
	return config
}

type Locker struct {
	l          log.Logger
	config     Config
	mlock      mlockclient.Locker
	mlockRetry *retry.BackOff
}

var _ locker.Locker = &Locker{}

func New(config Config, mlock mlockclient.Locker, l log.Logger) *Locker {
	return &Locker{
		l:          l,
		config:     config,
		mlock:      mlock,
		mlockRetry: retry.New(config.Retries),
	}
}

func (lck *Locker) Acquire(ctx context.Context, lockID string, fqdns []string, reason string) (err error) {
	lockLogger := log.With(lck.l, log.String("lockID", lockID))
	createCtx, createCancel := context.WithTimeout(ctx, lck.config.CreateTimeout.Duration)
	defer func() {
		if r := recover(); r != nil {
			_ = lck.Release(ctx, lockID)
			panic(r)
		}
		if err != nil {
			_ = lck.Release(ctx, lockID)
		}
	}()
	defer createCancel()
	err = lck.mlockRetry.RetryWithLog(
		createCtx,
		func() error {
			return lck.mlock.CreateLock(createCtx, lockID, holder, fqdns, reason)
		},
		"create lock",
		lockLogger,
	)
	if err != nil {
		return err
	}

	var lastConflicts []mlockclient.Conflict
	startWaitAt := time.Now()
	for tryNum := 1; tryNum <= lck.config.CheckAttempts; tryNum++ {
		status, statusErr := lck.mlock.GetLockStatus(ctx, lockID)
		// don't need `statusErr == nil` check,
		// but Goland complaints about it:
		//	'status' may have 'nil' or other unexpected value as its corresponding error variable may be not 'nil'
		if statusErr == nil && status.Acquired {
			lockLogger.Infof("lock acquired in %s", time.Since(startWaitAt))
			return nil
		}
		if statusErr != nil {
			lockLogger.Warnf("get status failed: %s", statusErr)
		} else {
			lastConflicts = status.Conflicts
			lockLogger.Debugf("waiting for lock. Conflicts: %+v. It's %d try", status.Conflicts, tryNum)
		}
		time.Sleep(lck.config.CheckInterval.Duration)
	}

	return xerrors.Errorf(
		"couldn't acquire %q lock in %s timeout. Conflicts: %+v",
		lockID,
		time.Since(startWaitAt),
		lastConflicts,
	)
}

func (lck *Locker) Release(ctx context.Context, lockID string) error {
	lockLogger := log.With(lck.l, log.String("lockID", lockID))
	return lck.mlockRetry.RetryWithLog(
		ctx,
		func() error {
			if err := lck.mlock.ReleaseLock(ctx, lockID); err != nil {
				if semerr.IsNotFound(err) {
					lockLogger.Debug("lock not found. Probably it's Release retry")
					return nil
				}
				return err
			}
			return nil
		},
		"release lock",
		lockLogger,
	)
}
