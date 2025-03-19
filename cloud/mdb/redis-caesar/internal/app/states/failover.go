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

type StateFailover struct {
	conf      *config.Global
	logger    log.Logger
	dcs       dcs.DCS
	db        database.DB
	oldMaster internal.RedisHost
}

func NewStateFailover(conf *config.Global, logger log.Logger, dcs dcs.DCS, db database.DB, master internal.RedisHost, timestamp int64) *StateFailover {
	return &StateFailover{
		conf:      conf,
		logger:    logger,
		dcs:       dcs,
		db:        db,
		oldMaster: master,
	}
}

func (s *StateFailover) Run() (AppState, error) {
	if !s.dcs.IsConnected() {
		return NewStateLost(s.conf, s.logger, s.dcs), nil
	}
	if !s.dcs.AcquireManagerLock() {
		return NewStateCandidate(s.conf, s.logger, s.dcs)
	}

	err := s.db.PingHost(context.TODO(), s.oldMaster)
	if err == nil {
		return NewStateManager(s.conf, s.logger, s.dcs)
	}

	dbsInfo, err := s.db.DatabasesInfo(context.TODO())
	if err != nil {
		return nil, fmt.Errorf("unable to retrieve database info for failover: %w", err)
	}

	var maxReplicaOffset int64
	master := internal.RedisHostEmpty

	for host, info := range dbsInfo {
		if info.ReplicaOffset >= maxReplicaOffset && host != s.oldMaster {
			master = host
		}
	}
	if master == internal.RedisHostEmpty {
		return nil, fmt.Errorf("unable to elect a new master")
	}

	err = s.db.PromoteMaster(context.TODO(), master)
	if err != nil {
		return nil, fmt.Errorf("unable to promote master: %w", err)
	}

	return NewStateManager(s.conf, s.logger, s.dcs)
}

func (s *StateFailover) Name() string {
	return "StateFailover"
}
