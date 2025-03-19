package states

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/dcs/zookeeper"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

// StateFirstRun is a state that will will be entered when caesar starts.
type StateFirstRun struct {
	conf   *config.Global
	logger log.Logger
}

func NewStateFirstRun(conf *config.Global, logger log.Logger) *StateFirstRun {
	return &StateFirstRun{
		conf:   conf,
		logger: logger,
	}
}

// Run is an implementation of state interface.
func (s *StateFirstRun) Run() (AppState, error) {
	dcs, err := zookeeper.New(s.conf.ZooKeeper, s.logger)
	if err != nil {
		return nil, fmt.Errorf("state first run - unable to connect to DCS: %w", err)
	}

	s.logger.Debug("connecting to DCS")
	if !dcs.WaitConnected(s.conf.DCSWaitTimeout) {
		s.logger.Debug("unable to connect to DCS: timeout exceeded", log.Duration("timeout", s.conf.DCSWaitTimeout))

		return s, nil
	}
	dcs.Initialize()
	if dcs.AcquireManagerLock() {
		stateManager, err := NewStateManager(s.conf, s.logger, dcs)
		if err != nil {
			return nil, fmt.Errorf("unable to create state manager: %w", err)
		}

		return stateManager, nil
	}

	stateCandidate, err := NewStateCandidate(s.conf, s.logger, dcs)
	if err != nil {
		return nil, fmt.Errorf("unable to create state candidate: %w", err)
	}

	return stateCandidate, nil
}

func (s *StateFirstRun) Name() string {
	return "StateFirstRun"
}
