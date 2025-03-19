package states

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/database"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/dcs"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

type StateFailoverWaiting struct {
	clock          internal.Clock
	conf           *config.Global
	logger         log.Logger
	dcs            dcs.DCS
	db             database.DB
	startTimestamp int64
	master         internal.RedisHost
}

func NewStateFailoverWaiting(conf *config.Global, logger log.Logger, dcs dcs.DCS, db database.DB, master internal.RedisHost, timestamp int64) *StateFailoverWaiting {
	clock := &internal.RealClock{}
	if timestamp == internal.TimestampEmpty {
		clock.Now()
	}

	return &StateFailoverWaiting{
		clock:          clock,
		conf:           conf,
		logger:         logger,
		dcs:            dcs,
		db:             db,
		startTimestamp: timestamp,
		master:         master,
	}
}

func (s *StateFailoverWaiting) Run() (AppState, error) {
	if !s.dcs.IsConnected() {
		return NewStateLost(s.conf, s.logger, s.dcs), nil
	}
	if !s.dcs.AcquireManagerLock() {
		return NewStateCandidate(s.conf, s.logger, s.dcs)
	}

	err := s.dcs.StartFailoverWait(s.startTimestamp, s.master)
	if err != nil {
		return nil, fmt.Errorf("unable to write data about failover wait: %w", err)
	}

	pingErr := s.db.PingHost(context.TODO(), s.master)

	// if we can ping host then go to manager state
	if pingErr == nil {
		return NewStateManager(s.conf, s.logger, s.dcs)
	}

	if s.clock.TimeoutPassed(s.startTimestamp, s.conf.PingTimeout) {
		return NewStateFailover(s.conf, s.logger, s.dcs, s.db, s.master, internal.TimestampEmpty), nil
	}

	return s, nil
}

func (s *StateFailoverWaiting) Name() string {
	return "StateFailoverWaiting"
}
