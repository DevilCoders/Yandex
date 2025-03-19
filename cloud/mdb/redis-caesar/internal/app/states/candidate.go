package states

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/database"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/database/redis"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/dcs"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

type StateCandidate struct {
	conf   *config.Global
	logger log.Logger
	dcs    dcs.DCS
	db     database.DB
}

func NewStateCandidate(conf *config.Global, logger log.Logger, dcs dcs.DCS) (*StateCandidate, error) {
	db, err := redis.New(conf.Redis, logger)
	if err != nil {
		return nil, fmt.Errorf("unable to connect to database: %w", err)
	}

	return &StateCandidate{
		conf:   conf,
		logger: logger,
		dcs:    dcs,
		db:     db,
	}, nil
}

func (s *StateCandidate) Run() (AppState, error) {
	if !s.dcs.IsConnected() {
		return NewStateLost(s.conf, s.logger, s.dcs), nil
	}

	info, err := s.dcs.DatabasesInfo()

	if err != nil {
		return nil, fmt.Errorf("unable to get databases info from dcs: %w", err)
	}

	err = s.db.UpdateHostsConnections(context.TODO(), info)
	if err != nil {
		return s, fmt.Errorf("candidate: failed to update host info zk %w", err)
	}
	if s.dcs.AcquireManagerLock() {
		stateManager, err := NewStateManager(s.conf, s.logger, s.dcs)
		if err != nil {
			return nil, fmt.Errorf("unable to create state manager: %w", err)
		}

		return stateManager, nil
	}

	return s, nil
}

func (s *StateCandidate) Name() string {
	return "StateCandidate"
}
