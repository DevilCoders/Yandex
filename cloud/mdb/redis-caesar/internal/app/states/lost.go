package states

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/dcs"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

type StateLost struct {
	conf   *config.Global
	logger log.Logger
	dcs    dcs.DCS
}

func NewStateLost(conf *config.Global, logger log.Logger, dcs dcs.DCS) *StateLost {
	return &StateLost{
		conf:   conf,
		logger: logger,
		dcs:    dcs,
	}
}

func (s *StateLost) Run() (AppState, error) {
	if s.dcs.IsConnected() {
		stateCandidate, err := NewStateCandidate(s.conf, s.logger, s.dcs)
		if err != nil {
			return nil, fmt.Errorf("unable to create state candidate: %w", err)
		}

		return stateCandidate, nil
	}

	s.logger.Warn("caesar have lost connection to ZK.")

	return s, nil
}

func (s *StateLost) Name() string {
	return "StateLost"
}
