package app

import (
	"context"
	"fmt"
	"os"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/scheduler"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/scheduler/generic"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/scheduler/sequential"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	AppName = "scheduler"
)

var (
	PlanTags     = map[string]string{"action": "plan"}
	ObsoleteTags = map[string]string{"action": "obsolete"}
	PurgeTags    = map[string]string{"action": "purge"}
)

type SchedulerStat struct {
	Duration metrics.TimerVec
	Handled  metrics.CounterVec
	ExitCode metrics.CounterVec
}

type RunOpts struct {
	Plan     bool
	Obsolete bool
	Purge    bool
}

type Config struct {
	App         app.Config            `json:"app" yaml:"app"`
	Scheduler   generic.Config        `json:"scheduler" yaml:"scheduler"`
	Metadb      pgutil.Config         `json:"metadb" yaml:"metadb"`
	InitTimeout encodingutil.Duration `json:"init_timeout" yaml:"init_timeout"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App: app.DefaultConfig(),
		Metadb: pgutil.Config{
			User: "backup_scheduler",
			DB:   mdbpg.DBName,
		},
		Scheduler:   generic.DefaultConfig(),
		InitTimeout: encodingutil.Duration{Duration: time.Second * 10},
	}
	cfg.App.ServiceAccount.FromEnv("BACKUP_SA")
	return cfg
}

type SchedulerApp struct {
	*app.App
	Config Config
	MDB    metadb.MetaDB
	Sched  scheduler.Scheduler
	stat   SchedulerStat
}

// NewAppFromConfig builds scheduler from config
func NewAppFromConfig(ctx context.Context, clusterTypes []metadb.ClusterType) (*SchedulerApp, error) {
	conf := DefaultConfig()
	opts := app.DefaultToolOptions(&conf, fmt.Sprintf("%s.yaml", AppName))
	opts = append(opts, app.WithMetrics())
	opts = append(opts, app.WithMetricsSendOnShutdown())

	baseApp, err := app.New(opts...)
	if err != nil {
		return nil, xerrors.Errorf("cannot make base app, %w", err)
	}
	logger := baseApp.L()

	if len(clusterTypes) != 1 {
		return nil, xerrors.Errorf("expected one cluster_type but given %d: %+v ", len(clusterTypes), clusterTypes)
	}
	clusterType := clusterTypes[0]

	if !conf.Metadb.Password.FromEnv("METADB_PASSWORD") {
		logger.Info("METADB_PASSWORD is empty")
	}
	mdb, err := mdbpg.New(conf.Metadb, log.With(logger, log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return nil, xerrors.Errorf("failed to prepare MetaDB endpoint: %w", err)
	}
	// TODO: wrap scheduler into app and support sentry
	sched, err := NewSchedulerFromExtDeps(clusterType, mdb, logger, &generator.BackupIDGenerator{}, conf.Scheduler)
	if err != nil {
		return nil, err
	}
	if err = ready.Wait(ctx, sched, &ready.DefaultErrorTester{Name: "metadb", L: logger}, conf.InitTimeout.Duration); err != nil {
		return nil, xerrors.Errorf("failed to wait backend: %w", err)
	}

	solomonTags := map[string]string{
		"cluster_type": string(clusterType),
	}
	stat := SchedulerStat{
		Duration: baseApp.Metrics().WithTags(solomonTags).TimerVec("mdb_backup_scheduler_duration", []string{"action"}),
		Handled:  baseApp.Metrics().WithTags(solomonTags).CounterVec("mdb_backup_scheduler_handled", []string{"action"}),
		ExitCode: baseApp.Metrics().WithTags(solomonTags).CounterVec("mdb_backup_scheduler_exit_code", []string{"action"}),
	}

	a := &SchedulerApp{
		App:    baseApp,
		Config: conf,
		MDB:    mdb,
		Sched:  sched,
		stat:   stat,
	}
	return a, nil
}

func NewSchedulerFromExtDeps(clusterType metadb.ClusterType, mdb metadb.MetaDB, logger log.Logger, idGen generator.IDGenerator, cfg generic.Config) (scheduler.Scheduler, error) {
	if clusterType == metadb.ClickhouseCluster {
		return sequential.NewSchedulerWithSequentialDeletion([]metadb.ClusterType{clusterType}, mdb, logger, idGen, cfg)
	} else {
		return generic.NewGenericScheduler([]metadb.ClusterType{clusterType}, mdb, logger, idGen, cfg)
	}
}

func (sa *SchedulerApp) runPlan(ctx context.Context, dryRun bool) error {
	started := time.Now()
	scheduled, err := sa.Sched.PlanBackups(ctx, dryRun)
	sa.stat.Duration.With(PlanTags).RecordDuration(time.Since(started))

	if err != nil {
		sa.L().Error("Failed to generate backups", log.Error(err))
		sa.stat.ExitCode.With(PlanTags).Add(1)
		return err
	}

	sa.L().Infof("schedule-create completed successfully (dryRun=%t): %+v", dryRun, scheduled)
	if !dryRun {
		sa.stat.Handled.With(PlanTags).Add(scheduled.Planned)
	}
	sa.stat.ExitCode.With(PlanTags).Add(0)
	return nil
}

func (sa *SchedulerApp) runObsolete(ctx context.Context, dryRun bool) error {
	started := time.Now()
	deleted, err := sa.Sched.ObsoleteBackups(ctx, dryRun)
	sa.stat.Duration.With(ObsoleteTags).RecordDuration(time.Since(started))

	if err != nil {
		sa.L().Error("Failed to mark outdated backups", log.Error(err))
		sa.stat.ExitCode.With(ObsoleteTags).Add(1)
		return err
	}

	sa.L().Infof("schedule-obsolete completed successfully (dryRun=%t): %+v", dryRun, deleted)
	if !dryRun {
		sa.stat.Handled.With(ObsoleteTags).Add(deleted.Obsolete)
	}
	sa.stat.ExitCode.With(ObsoleteTags).Add(0)
	return nil
}

func (sa *SchedulerApp) runPurge(ctx context.Context, dryRun bool) error {
	started := time.Now()
	purged, err := sa.Sched.PurgeBackups(ctx, dryRun)
	sa.stat.Duration.With(PurgeTags).RecordDuration(time.Since(started))

	if err != nil {
		sa.L().Error("failed to purge deleted backups", log.Error(err))
		sa.stat.ExitCode.With(PurgeTags).Add(1)
		return err
	}
	sa.L().Infof("run-purge completed successfully (dryRun=%t): %+v", dryRun, purged)
	if !dryRun {
		sa.stat.Handled.With(PurgeTags).Add(purged.Purged)
	}
	sa.stat.ExitCode.With(PurgeTags).Add(0)
	return nil
}

func Run(ctx context.Context, clusterTypes []metadb.ClusterType, opts RunOpts, dryRun bool) int {
	a, err := NewAppFromConfig(ctx, clusterTypes)
	if err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "scheduler Failed to start app: %+v\n", err)
		return 1
	}

	var errs []error
	a.L().Infof("scheduler started with dryRun = %t", dryRun)
	defer a.App.Shutdown()

	if opts.Plan {
		errs = append(errs, a.runPlan(ctx, dryRun))
	}

	if opts.Obsolete {
		errs = append(errs, a.runObsolete(ctx, dryRun))
	}

	if opts.Purge {
		errs = append(errs, a.runPurge(ctx, dryRun))
	}

	for _, err := range errs {
		if err != nil {
			a.L().Errorf("scheduler has completed with errors. See log above.")
			return 11
		}
	}
	a.L().Infof("scheduler has completed successfully")
	return 0
}
