package rediscachetier

import (
	"bytes"
	"context"
	"io/ioutil"
	"time"

	"go.uber.org/atomic"

	"a.yandex-team.ru/library/go/core/log"
)

const (
	redisTierMaster = "master"
)

type Config struct {
	// Time-to-live of leader lock. Also used for calculate leader check period
	LeaderTTL time.Duration

	// Internal params

	// Do not start elect timer
	turnOffElectCycle bool

	// File for tier, in config for tests
	redisTierCacheFile string
}

func DefaultConfig() Config {
	return Config{
		LeaderTTL: 15 * time.Second,
	}
}

type LeaderElector struct {
	cfg      Config
	logger   log.Logger
	isLeader *atomic.Bool
}

// New creates instance of elector. This instance will be useful only for current architecture: node with redis and api.
// Deprecated: For k8s use redsync implementation. This is for temp purposes.
func New(ctx context.Context, cfg Config, logger log.Logger) *LeaderElector {
	if cfg.redisTierCacheFile == "" {
		cfg.redisTierCacheFile = "/tmp/.redis_tier.cache"
	}
	le := &LeaderElector{
		cfg:      cfg,
		logger:   logger,
		isLeader: atomic.NewBool(false),
	}
	if !cfg.turnOffElectCycle {
		go le.refresher(ctx)
	}
	return le
}

func (le *LeaderElector) refresher(ctx context.Context) {
	delay := le.cfg.LeaderTTL / 3
	ticker := time.NewTicker(delay)
	defer ticker.Stop()
	for {
		select {
		case <-ticker.C:
			le.refresh()
		case <-ctx.Done():
			ticker.Stop()
			return
		}
	}
}

func (le *LeaderElector) refresh() {
	data, err := ioutil.ReadFile(le.cfg.redisTierCacheFile)
	if err != nil {
		le.logger.Warn("failed to read redis tier cache file", log.String("file", le.cfg.redisTierCacheFile), log.Error(err))
		le.isLeader.Swap(false)
		return
	}

	currentLeadership := bytes.Equal(data, []byte(redisTierMaster))
	le.logger.Debug("current leadership", log.Bool("newIsLeader", currentLeadership), log.Error(err))
	if le.isLeader.CAS(le.isLeader.Load(), currentLeadership) {
		le.logger.Warn("leader has been changes or initialized", log.Bool("newIsLeader", currentLeadership), log.Error(err))
	}
}

func (le *LeaderElector) IsLeader(_ context.Context) bool {
	return le.isLeader.Load()
}
