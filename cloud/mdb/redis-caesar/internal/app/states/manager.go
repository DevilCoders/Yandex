package states

import (
	"context"
	"errors"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/database"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/database/redis"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/dcs"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

type StateManager struct {
	conf   *config.Global
	logger log.Logger
	dcs    dcs.DCS
	db     database.DB
}

func NewStateManager(conf *config.Global, logger log.Logger, dcs dcs.DCS) (*StateManager, error) {
	db, err := redis.New(conf.Redis, logger)
	if err != nil {
		return nil, fmt.Errorf("unable to connect to database: %w", err)
	}

	return &StateManager{
		conf:   conf,
		logger: logger,
		dcs:    dcs,
		db:     db,
	}, nil
}

func (s *StateManager) Run() (AppState, error) {
	if !s.dcs.IsConnected() {
		return NewStateLost(s.conf, s.logger, s.dcs), nil
	}
	if !s.dcs.AcquireManagerLock() {
		return NewStateCandidate(s.conf, s.logger, s.dcs)
	}

	s.db.InitHostsConnections(context.TODO())

	failloverWaitStarted, err := s.dcs.FailoverWaitStarted()
	if err == nil { // if we found in DCS that we already started failover waiting then we need to resume waiting
		return NewStateFailoverWaiting(s.conf, s.logger, s.dcs, s.db, failloverWaitStarted.OldMaster, failloverWaitStarted.Timestamp), nil
	} else if !errors.As(err, &dcs.NotFoundError{}) {
		return s, fmt.Errorf("unable to write failover started data to DCS: %w", err)
	}

	failloverStarted, err := s.dcs.FailoverWaitStarted()
	if err == nil { // if we found in DCS that we already started failover then we need to resume failover
		return NewStateFailover(s.conf, s.logger, s.dcs, s.db, failloverStarted.OldMaster, failloverStarted.Timestamp), nil
	} else if !errors.As(err, &dcs.NotFoundError{}) {
		return s, fmt.Errorf("unable to write failover started data to DCS: %w", err)
	}

	master, err := s.dcs.DBMaster()
	if err != nil {
		return s, fmt.Errorf("unable to get database master from DCS: %w", err)
	}

	// If we do not have a master saved in DCS that means that this cluster is never been initialized so we need to initialize it
	if master == internal.RedisHostEmpty {
		err = s.db.Ping(context.TODO())
		if err != nil {
			if errors.As(err, &database.PingError{}) {
				return NewStateRedisWaiting(), nil
			}

			return nil, fmt.Errorf("unable to ping databases, %w", err)
		}

		err = s.initDatabaseCluster()
		if err != nil {
			return s, fmt.Errorf("unable to init database cluster, %w", err)
		}
	}

	info, err := s.db.DatabasesInfo(context.TODO())
	if err != nil {
		return nil, fmt.Errorf("unable to retrieve health information from databases: %w", err)
	}

	dcsInfo, err := s.dcs.DatabasesInfo()
	if err != nil {
		return nil, fmt.Errorf("unable to retrieve health information from DCS: %w", err)
	}

	needFailover := s.compareDatabaseInfos(info, dcsInfo)
	if needFailover {
		s.dcs.AcquireManagerLock()

		return NewStateFailoverWaiting(s.conf, s.logger, s.dcs, s.db, master, internal.TimestampEmpty), nil
	}

	return s, nil
}

func (s *StateManager) Name() string {
	return "StateManager"
}

func (s *StateManager) initDatabaseCluster() error {
	dbsInfo, err := s.db.DatabasesInfo(context.TODO())
	if err != nil {
		return fmt.Errorf("unable to get databases info: %w", err)
	}
	var maxReplicaOffset int64
	master := internal.RedisHostEmpty
	// finding host with most actual replica
	for host, info := range dbsInfo {
		if info.ReplicaOffset >= maxReplicaOffset {
			master = host
		}
	}
	if master == internal.RedisHostEmpty {
		return fmt.Errorf("unable to elect a new master")
	}

	err = s.db.PromoteMaster(context.TODO(), master)
	if err != nil {
		return fmt.Errorf("unable to promote master: %w", err)
	}

	return nil
}

// compareDatabaseInfos checks the information from database provider and DCS.
func (s *StateManager) compareDatabaseInfos(dbInfo, dcsInfo internal.AllDBsInfo) (needFailover bool) {
	result := false
	for host, hostInfo := range dbInfo {
		if !hostInfo.Alive {
			if dcsInfo[host].Role == internal.RedisRoleMaster {
				result = true
			}
		}
	}

	return result
}
