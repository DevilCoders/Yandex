package generic

import (
	"context"
	"math/rand"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/argsextractor"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/argsextractor/provider"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	Plain       = "PLAIN"
	Incremental = "INCREMENTAL"
)

// ScheduleRules ...
type ScheduleRules struct {
	ScheduleMode       string   `json:"schedule_mode" yaml:"schedule_mode"`
	ExcludeSubclusters []string `json:"exclude_subclusters" yaml:"exclude_subclusters"`
}

// ScheduleConfig describes configuration for backup scheduler.
type ScheduleConfig struct {
	FutureInterval   encodingutil.Duration                `json:"future_interval" yaml:"future_interval"`
	PastInterval     encodingutil.Duration                `json:"past_interval" yaml:"past_interval"`
	ClusterTypeRules map[metadb.ClusterType]ScheduleRules `json:"cluster_type_rules" yaml:"cluster_type_rules"`
}

// ObsoleteConfig describes configuration for backup cleaner.
type ObsoleteConfig struct {
	ObsoleteFailedAfter encodingutil.Duration `json:"obsolete_failed_after" yaml:"obsolete_failed_after"`
}

// PurgeConfig describes configuration for backup cleaner.
type PurgeConfig struct {
	PurgeObsoletedAfter encodingutil.Duration `json:"purge_obsoleted_after" yaml:"purge_obsoleted_after"`
}

// Config ...
type Config struct {
	ScheduleConfig ScheduleConfig `json:"schedule_config" yaml:"schedule_config"`
	ObsoleteConfig ObsoleteConfig `json:"obsolete_config" yaml:"obsolete_config"`
	PurgeConfig    PurgeConfig    `json:"purge_config" yaml:"purge_config"`
}

func DefaultConfig() Config {
	return Config{
		ScheduleConfig: ScheduleConfig{
			FutureInterval: encodingutil.Duration{Duration: 24 * time.Hour},
			PastInterval:   encodingutil.Duration{Duration: 60 * time.Minute},
			ClusterTypeRules: map[metadb.ClusterType]ScheduleRules{
				metadb.MongodbCluster: {
					ExcludeSubclusters: []string{"mongos_subcluster"},
					ScheduleMode:       Plain,
				},
				metadb.PostgresqlCluster: {
					ScheduleMode: Incremental,
				},
				metadb.MysqlCluster: {
					ScheduleMode: Plain,
				},
				metadb.ClickhouseCluster: {
					ScheduleMode: Plain,
				},
			},
		},
		ObsoleteConfig: ObsoleteConfig{
			ObsoleteFailedAfter: encodingutil.Duration{Duration: 14 * 24 * time.Hour},
		},
		PurgeConfig: PurgeConfig{
			PurgeObsoletedAfter: encodingutil.Duration{Duration: 14 * 24 * time.Hour},
		},
	}
}

func shouldScheduleBackup(blank metadb.BackupBlank, rules map[metadb.ClusterType]ScheduleRules) bool {
	for _, subcluster := range rules[blank.ClusterType].ExcludeSubclusters {
		if subcluster == blank.SubClusterName {
			return false
		}
	}
	return true
}

// GenericScheduler ...
type GenericScheduler struct {
	Mdb          metadb.MetaDB
	BackupIDGen  generator.IDGenerator
	Lg           log.Logger
	Cfg          Config
	ClusterTypes []metadb.ClusterType
}

// NewGenericScheduler creates new GenericScheduler.
func NewGenericScheduler(clusterTypes []metadb.ClusterType, mdb metadb.MetaDB, lg log.Logger, idGen generator.IDGenerator, cfg Config) (*GenericScheduler, error) {
	if len(clusterTypes) < 1 {
		return nil, xerrors.Errorf("empty cluster types list")
	}
	for _, ct := range clusterTypes {
		if _, ok := cfg.ScheduleConfig.ClusterTypeRules[ct]; !ok {
			return nil, xerrors.Errorf("cluster type is not configured in scheduler config: %s", ct)
		}
	}
	return &GenericScheduler{
		Mdb:          mdb,
		Lg:           lg,
		BackupIDGen:  idGen,
		Cfg:          cfg,
		ClusterTypes: clusterTypes,
	}, nil
}

func (s *GenericScheduler) IsReady(ctx context.Context) error {
	if err := s.Mdb.IsReady(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, "metadb is not ready")
	}
	return nil
}

// PlanBackups creates backup records.
// TODO: move dryrun arg to constructor
func (s *GenericScheduler) PlanBackups(ctx context.Context, dryrun bool) (models.PlannedStats, error) {
	var scheduled int64 = 0
	cfg := s.Cfg.ScheduleConfig
	ctx, err := s.Mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return models.PlannedStats{}, err
	}
	defer s.Mdb.Rollback(ctx)

	blanks, err := s.Mdb.GetBackupBlanks(ctx, s.ClusterTypes, cfg.PastInterval.Duration, cfg.FutureInterval.Duration)
	if err != nil {
		return models.PlannedStats{}, err
	}
	rand.Seed(time.Now().UnixNano())

	extractors := map[string]argsextractor.BackupArgsExtractor{
		Plain:       provider.NewGenericBackupArgsExtractor(),
		Incremental: provider.NewIncrementalBackupArgsExtractor(s.Mdb, s.Lg),
	}

	for _, blank := range blanks {
		rule, ok := cfg.ClusterTypeRules[blank.ClusterType]
		if !ok {
			continue
		}
		if !shouldScheduleBackup(blank, cfg.ClusterTypeRules) {
			continue
		}

		e, ok := extractors[rule.ScheduleMode]
		if !ok {
			return models.PlannedStats{}, xerrors.New("unknown schedule mode")
		}

		backupArgs, err := e.GetBackupArgs(ctx, blank, s.BackupIDGen)
		if err != nil {
			return models.PlannedStats{}, err
		}

		s.Lg.Debugf("Fetched backup to schedule: %+v", backupArgs)
		if _, err = s.Mdb.AddBackup(ctx, backupArgs); err != nil {
			return models.PlannedStats{}, err
		}
		scheduled++
	}

	if dryrun {
		return models.PlannedStats{SkippedDueToDryRun: scheduled}, nil
	}

	if err := s.Mdb.Commit(ctx); err != nil {
		s.Lg.Errorf("Failed to commit generated backups: %v", err)
		return models.PlannedStats{}, err
	}
	return models.PlannedStats{Planned: scheduled}, nil
}

// ObsoleteBackups marks outdated backups for deletion.
func (s *GenericScheduler) ObsoleteBackups(ctx context.Context, dryrun bool) (models.ObsoleteStats, error) {
	ctx, err := s.Mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return models.ObsoleteStats{}, err
	}
	defer s.Mdb.Rollback(ctx)

	nAutomated, err := s.Mdb.ObsoleteAutomatedBackups(ctx, s.ClusterTypes)
	if err != nil {
		return models.ObsoleteStats{}, err
	}

	nFailed, err := s.Mdb.ObsoleteFailedBackups(ctx, s.ClusterTypes, s.Cfg.ObsoleteConfig.ObsoleteFailedAfter.Duration)
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

// PurgeBackups removes metadata of deleted backups.
func (s *GenericScheduler) PurgeBackups(ctx context.Context, dryrun bool) (models.PurgeStats, error) {
	if len(s.ClusterTypes) < 1 {
		return models.PurgeStats{}, xerrors.Errorf("empty cluster types list")
	}
	ctx, err := s.Mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return models.PurgeStats{}, err
	}
	defer s.Mdb.Rollback(ctx)

	n, err := s.Mdb.PurgeDeletedBackups(ctx, s.ClusterTypes, s.Cfg.PurgeConfig.PurgeObsoletedAfter.Duration)
	if err != nil {
		return models.PurgeStats{}, err
	}

	if dryrun {
		return models.PurgeStats{SkippedDueToDryRun: n}, nil
	}

	if err := s.Mdb.Commit(ctx); err != nil {
		return models.PurgeStats{}, err
	}
	return models.PurgeStats{Purged: n}, nil
}
