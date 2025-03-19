package sequential

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/models"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/scheduler/generic"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

/*
	This Scheduler marks backups as Obsolete in sequatinal manner per cluster.
	i.e it will not mark backup Obsolete in case cluster already has backup in Obsolete/Deleting status
*/
type SchedulerWithSequentialDeletion struct {
	generic.GenericScheduler
}

// NewSchedulerWithSequentialDeletion creates new SchedulerWithSequentialDeletion.
func NewSchedulerWithSequentialDeletion(clusterTypes []metadb.ClusterType, mdb metadb.MetaDB, lg log.Logger, idGen generator.IDGenerator, cfg generic.Config) (*SchedulerWithSequentialDeletion, error) {
	if len(clusterTypes) < 1 {
		return nil, xerrors.Errorf("empty cluster types list")
	}
	for _, ct := range clusterTypes {
		if _, ok := cfg.ScheduleConfig.ClusterTypeRules[ct]; !ok {
			return nil, xerrors.Errorf("cluster type is not configured in scheduler config: %s", ct)
		}
	}
	return &SchedulerWithSequentialDeletion{
		generic.GenericScheduler{
			Mdb:          mdb,
			Lg:           lg,
			BackupIDGen:  idGen,
			Cfg:          cfg,
			ClusterTypes: clusterTypes,
		},
	}, nil
}

// ObsoleteBackups marks outdated backups for deletion.
func (s *SchedulerWithSequentialDeletion) ObsoleteBackups(ctx context.Context, dryrun bool) (models.ObsoleteStats, error) {
	ctx, err := s.Mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return models.ObsoleteStats{}, err
	}
	defer s.Mdb.Rollback(ctx)

	nAutomated, err := s.Mdb.SequentialObsoleteAutomatedBackups(ctx, s.ClusterTypes)
	if err != nil {
		return models.ObsoleteStats{}, err
	}

	nFailed, err := s.Mdb.SequentialObsoleteFailedBackups(ctx, s.ClusterTypes, s.Cfg.ObsoleteConfig.ObsoleteFailedAfter.Duration)
	if err != nil {
		return models.ObsoleteStats{}, err
	}

	if dryrun {
		return models.ObsoleteStats{SkippedDueToDryRun: nAutomated + nFailed}, nil
	}

	if err := s.Mdb.Commit(ctx); err != nil {
		return models.ObsoleteStats{}, err
	}
	return models.ObsoleteStats{Obsolete: nAutomated + nFailed}, nil
}
