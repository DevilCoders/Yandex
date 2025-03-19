package redsync

import (
	"context"
	"fmt"
	"time"

	"github.com/go-redis/redis/v8"
	"github.com/go-redsync/redsync/v4"
	"github.com/go-redsync/redsync/v4/redis/goredis/v8"

	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	// Application name
	AppName string
	// Time-to-live of leader lock. Also used for calculate leader check period
	LeaderTTL time.Duration
	// The host name or other unique name of the application instance
	InstanceID string
	// Type of database
	Type string

	// Internal params

	// Do not start elect timer
	turnOffElectCycle bool
}

func DefaultConfig() Config {
	return Config{
		LeaderTTL:  15 * time.Second,
		InstanceID: "common",
		Type:       "all",
		AppName:    "app",
	}
}

type LeaderElector struct {
	mut    *redsync.Mutex
	cfg    Config
	logger log.Logger
}

func New(ctx context.Context, redis redis.UniversalClient, cfg Config, logger log.Logger) *LeaderElector {
	mutexOptions := []redsync.Option{
		redsync.WithExpiry(cfg.LeaderTTL),
		redsync.WithGenValueFunc(func() (string, error) {
			return cfg.InstanceID, nil
		}),
		redsync.WithTries(3),
		redsync.WithRetryDelay(time.Second),
	}
	rs := redsync.New(goredis.NewPool(redis))
	mut := rs.NewMutex(fmt.Sprintf("%s-%s-leader-election-lock", cfg.AppName, cfg.Type), mutexOptions...)
	le := &LeaderElector{
		mut:    mut,
		cfg:    cfg,
		logger: logger,
	}
	if !cfg.turnOffElectCycle {
		go le.refresher(ctx)
	}
	return le
}

func (le LeaderElector) refresher(ctx context.Context) {
	delay := le.cfg.LeaderTTL / 3
	ticker := time.NewTicker(delay)
	defer ticker.Stop()
	for {
		select {
		case <-ticker.C:
			le.reelect(ctx)
		case <-ctx.Done():
			ticker.Stop()
			return
		}
	}
}

func (le LeaderElector) reelect(ctx context.Context) {
	ok, err := le.mut.ValidContext(ctx)
	if err != nil {
		le.logger.Trace("Problem with valid lock check", log.Error(err))
	}

	if ok {
		le.logger.Trace("Mutex valid, try to extend it")
		ok, err := le.mut.ExtendContext(ctx)
		if err != nil {
			le.logger.Error("Can't extend mutex", log.Error(err))
		} else if !ok {
			le.logger.Error("Mutex is not valid, race")
		} else {
			le.logger.Trace("Mutex TTL is extend successfully")
			return
		}
	}

	if err = le.mut.LockContext(ctx); err != nil {
		le.logger.Trace("Can't lock mutex", log.Error(err))
	} else {
		le.logger.Info("We capture the leading successfully")
	}
}

func (le LeaderElector) IsLeader(ctx context.Context) bool {
	ok, err := le.mut.ValidContext(ctx)
	if err != nil {
		le.logger.Error("Error when leader check", log.Error(err))
	}
	return ok && err == nil
}
