package app

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/pg"
	backupmanagerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager/provider"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/delayer"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/executer"
	executerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/executer/provider"
	hostpicker "a.yandex-team.ru/cloud/mdb/backup/worker/internal/hostpicker"
	hostpickerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/hostpicker/provider"
	jobHandlerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/jobhandler/provider"
	jobProducerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/jobproducer/provider"
	queueHandlerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/queuehandler/provider"
	queueProducerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/queueproducer/provider"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/worker"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapirest "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	s3Prov "a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthswagger "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	AppName = "worker"
)

type PendingCycleConfig struct {
	Enabled         bool                          `json:"enabled" yaml:"enabled"`
	QueueProducer   queueProducerProv.Config      `json:"queue_producer" yaml:"queue_producer"`
	QueueHandler    queueHandlerProv.Config       `json:"queue_handler" yaml:"queue_handler"`
	JobHandler      jobHandlerProv.Config         `json:"job_handler" yaml:"job_handler"`
	FractionDelayer delayer.FractionDelayerConfig `json:"fraction_delayer" yaml:"fraction_delayer"`
}

func DefaultPendingCycleConfig() PendingCycleConfig {
	return PendingCycleConfig{
		Enabled:         false,
		QueueProducer:   queueProducerProv.DefaultConfig(),
		QueueHandler:    queueHandlerProv.DefaultConfig(),
		JobHandler:      jobHandlerProv.DefaultConfig(),
		FractionDelayer: delayer.DefaultFractionDelayerConfig(),
	}
}

type ExecutingCycleConfig struct {
	Enabled         bool                          `json:"enabled" yaml:"enabled"`
	QueueProducer   queueProducerProv.Config      `json:"queue_producer" yaml:"queue_producer"`
	QueueHandler    queueHandlerProv.Config       `json:"queue_handler" yaml:"queue_handler"`
	JobHandler      jobHandlerProv.Config         `json:"job_handler" yaml:"job_handler"`
	FractionDelayer delayer.FractionDelayerConfig `json:"fraction_delayer" yaml:"fraction_delayer"`
}

func DefaultExecutingCycleConfig() ExecutingCycleConfig {
	return ExecutingCycleConfig{
		Enabled:         false,
		QueueProducer:   queueProducerProv.DefaultConfig(),
		QueueHandler:    queueHandlerProv.DefaultConfig(),
		JobHandler:      jobHandlerProv.DefaultConfig(),
		FractionDelayer: delayer.DefaultFractionDelayerConfig(),
	}
}

type CyclesConfig struct {
	Planned  PendingCycleConfig   `json:"planned" yaml:"planned"`
	Creating ExecutingCycleConfig `json:"creating" yaml:"creating"`
	Obsolete PendingCycleConfig   `json:"obsolete" yaml:"obsolete"`
	Deleting ExecutingCycleConfig `json:"deleting" yaml:"deleting"`
}

func DefaultCyclesConfig() CyclesConfig {
	return CyclesConfig{
		Planned:  DefaultPendingCycleConfig(),
		Creating: DefaultExecutingCycleConfig(),
		Obsolete: DefaultPendingCycleConfig(),
		Deleting: DefaultExecutingCycleConfig(),
	}
}

type Config struct {
	App         app.Config                          `json:"app" yaml:"app"`
	Deploy      deployapirest.Config                `json:"deploy" yaml:"deploy"`
	Health      healthswagger.Config                `json:"health" yaml:"health"`
	S3Client    s3Prov.Config                       `json:"s3" yaml:"s3"`
	HostPicker  hostpickerProv.HostPickersMapConfig `json:"host_picker" yaml:"host_picker"`
	Metadb      pgutil.Config                       `json:"metadb" yaml:"metadb"`
	InitTimeout encodingutil.Duration               `json:"init_timeout" yaml:"init_timeout"`
	Worker      worker.Config                       `json:"worker" yaml:"worker"`
	Cycles      CyclesConfig                        `json:"cycles" yaml:"cycles"`
	Executer    executerProv.ConfigSet              `json:"executer" yaml:"executer"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App: app.DefaultConfig(),
		Metadb: pgutil.Config{
			User: "backup_worker",
			DB:   mdbpg.DBName,
		},
		Cycles:      DefaultCyclesConfig(),
		Worker:      worker.DefaultConfig(),
		Deploy:      deployapirest.DefaultConfig(),
		Health:      healthswagger.DefaultConfig(),
		S3Client:    s3Prov.Config{},
		Executer:    executerProv.DefaultConfig(),
		HostPicker:  hostpickerProv.DefaultConfig(),
		InitTimeout: encodingutil.FromDuration(time.Second * 10),
	}
	cfg.App.ServiceAccount.FromEnv("BACKUP_SA")
	return cfg
}

type DepsConfig struct {
	App        app.Config                          `json:"app" yaml:"app"`
	Deploy     deployapirest.Config                `json:"deploy" yaml:"deploy"`
	Health     healthswagger.Config                `json:"health" yaml:"health"`
	S3Client   s3Prov.Config                       `json:"s3" yaml:"s3"`
	HostPicker hostpickerProv.HostPickersMapConfig `json:"host_picker" yaml:"host_picker"`
	Metadb     pgutil.Config                       `json:"metadb" yaml:"metadb"`
}

func depsConfigFromWorker(cfg Config) DepsConfig {
	return DepsConfig{
		App:        cfg.App,
		Deploy:     cfg.Deploy,
		Health:     cfg.Health,
		S3Client:   cfg.S3Client,
		HostPicker: cfg.HostPicker,
		Metadb:     cfg.Metadb,
	}
}

type App struct {
	*app.App
	worker *worker.Worker
	L      log.Logger
}

func (app *App) waitForReady(ctx context.Context, initTimeout time.Duration) error {
	awaitCtx, cancel := context.WithTimeout(ctx, initTimeout)
	defer cancel()
	return ready.Wait(awaitCtx, app.worker, &ready.DefaultErrorTester{L: &nop.Logger{}}, time.Second)
}

func (app *App) Run(ctx context.Context) error {
	return app.worker.Run(ctx)
}

type ExternalDeps struct {
	Mdb      metadb.MetaDB
	Deploy   deployapi.Client
	Health   client.MDBHealthClient
	S3Client s3.Client
}

// NewExtDepsFromConfig builds worker from config
func NewExtDepsFromConfig(baseApp *app.App, cfg DepsConfig) (ExternalDeps, error) {
	lg := baseApp.L()

	if !cfg.Metadb.Password.FromEnv("METADB_PASSWORD") {
		lg.Info("METADB_PASSWORD is empty")
	}
	mdb, err := mdbpg.New(cfg.Metadb, log.With(lg, log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return ExternalDeps{}, xerrors.Errorf("failed to prepare MetaDB endpoint: %w", err)
	}

	if !cfg.App.ServiceAccount.Empty() && cfg.Deploy.Token.Unmask() != "" {
		return ExternalDeps{}, xerrors.Errorf("Deploy token and service account credentials must not be simultaneously")
	}
	if !cfg.App.ServiceAccount.FromEnv("BACKUP_SA") {
		lg.Info("BACKUP_SA_PRIVATE_KEY is not set in env")
	}

	var deployClient deployapi.Client
	if baseApp.ServiceAccountCredentials() != nil {
		deployClient, err = deployapirest.New(cfg.Deploy.URI, "", baseApp.ServiceAccountCredentials(), cfg.Deploy.Transport.TLS, cfg.Deploy.Transport.Logging, lg)
	} else {
		deployClient, err = deployapirest.NewFromConfig(cfg.Deploy, lg)
	}
	if err != nil {
		return ExternalDeps{}, xerrors.Errorf("failed to initialize Deploy client: %w", err)
	}

	// we don't need key, cause only read
	healthClient, err := healthswagger.NewClientTLSFromConfig(cfg.Health, lg)
	if err != nil {
		return ExternalDeps{}, xerrors.Errorf("failed to initialize Health client: %w", err)
	}

	if !cfg.S3Client.SecretKey.FromEnv("S3_SECRET_KEY") {
		lg.Infof("%s is empty", "S3_SECRET_KEY")
	}
	s3client, err := s3Prov.New(cfg.S3Client, lg)
	if err != nil {
		return ExternalDeps{}, err
	}

	return ExternalDeps{mdb, deployClient, healthClient, s3client}, nil
}

// NewAppFromConfig builds worker from config
func NewAppFromConfig(ctx context.Context) (*worker.Worker, log.Logger, error) {
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(&cfg, fmt.Sprintf("%s.yaml", AppName))...)
	if err != nil {
		return nil, nil, xerrors.Errorf("cannot make base app, %w", err)
	}
	lg := baseApp.L()

	deps, err := NewExtDepsFromConfig(baseApp, depsConfigFromWorker(cfg))
	if err != nil {
		return nil, nil, err
	}

	wkr, err := NewAppFromExtDeps(cfg, deps.Mdb, deps.Deploy, deps.Health, deps.S3Client, lg)
	if err != nil {
		return nil, nil, err
	}

	if err = ready.Wait(ctx, wkr, &ready.DefaultErrorTester{Name: "metadb", L: lg}, cfg.InitTimeout.Duration); err != nil {
		return nil, nil, xerrors.Errorf("failed to wait backend: %w", err)
	}

	return wkr, lg, nil
}

func hostPickerFromConfig(cfg hostpickerProv.Config, mdb metadb.MetaDB, health client.MDBHealthClient, lg log.Logger) hostpicker.HostPicker {
	pickerType := cfg.HostPickerType

	switch pickerType {
	case hostpickerProv.HealthyHostPickerType:
		return hostpickerProv.NewHealthyHostPicker(health, lg.WithName("HealthyHostPicker"), cfg)
	case hostpickerProv.PreferReplicaHostPickerType:
		healthServ := cfg.Config.ReplicationHealthService
		priorityArgs := hostpickerProv.PriorityArgs{
			PillarPath: cfg.Config.PriorityArgs,
		}
		return hostpickerProv.NewPreferReplicaHostPicker(mdb, health, lg.WithName("ReplicaHostPicker"), cfg, healthServ, priorityArgs)
	default:
		return nil
	}
}

func hostPickerFromCtype(cfg hostpickerProv.HostPickersMapConfig, mdb metadb.MetaDB, health client.MDBHealthClient, lg log.Logger, ctype metadb.ClusterType) hostpicker.HostPicker {
	pickerCfg := cfg.DefaultSettings
	if _, ok := cfg.ClusterTypeSettings[ctype]; ok {
		pickerCfg = cfg.ClusterTypeSettings[ctype]
	}

	return hostPickerFromConfig(pickerCfg, mdb, health, lg)
}

// NewAppFromExtDeps builds worker with external dependencies
func NewAppFromExtDeps(cfg Config, mdb metadb.MetaDB, deploy deployapi.Client, health client.MDBHealthClient, s3Client s3.Client, lg log.Logger) (*worker.Worker, error) {

	pgbm := backupmanagerProv.NewPostgreSQLBackupManager(s3Client, lg)
	mybm := backupmanagerProv.NewMySQLBackupManager(s3Client, lg)

	pgFactsExtr := backupmanagerProv.NewPostgreSQLJobFactsExtractor(mdb)
	pgArgProv := backupmanagerProv.NewPostgreSQLDeployArgsProvider(mdb, lg, pgFactsExtr)

	mongodbExecuter, err := executerProv.NewExecuter(mdb,
		lg.WithName("mongodb_executer"),
		deploy,
		hostPickerFromCtype(cfg.HostPicker, mdb, health, lg, metadb.MongodbCluster),
		backupmanagerProv.NewMongoDBBackupManager(s3Client, lg),
		nil,
		backupmanagerProv.NewMongoDBDeployArgsProvider(),
		backupmanagerProv.NewMongoDBJobFactsExtractor(),
		metadb.MongodbCluster,
		cfg.Executer.ByClusterType(metadb.MongodbCluster))
	if err != nil {
		return nil, xerrors.Errorf("mongodb executer: %w", err)
	}

	postgresqlExecuter, err := executerProv.NewExecuter(mdb,
		lg.WithName("postgresql_executer"),
		deploy,
		hostPickerFromCtype(cfg.HostPicker, mdb, health, lg, metadb.PostgresqlCluster),
		pgbm,
		backupmanagerProv.NewPostgresSizeMeasurer(pgbm, mdb, lg.WithName("postgresql_size_measurer")),
		pgArgProv,
		pgFactsExtr,
		metadb.PostgresqlCluster,
		cfg.Executer.ByClusterType(metadb.PostgresqlCluster))
	if err != nil {
		return nil, xerrors.Errorf("postgresql executer: %w", err)
	}

	mysqlExecuter, err := executerProv.NewExecuter(mdb,
		lg.WithName("mysql_executer"),
		deploy,
		hostPickerFromCtype(cfg.HostPicker, mdb, health, lg, metadb.MysqlCluster),
		mybm,
		nil,
		backupmanagerProv.NewMySQLDeployArgsProvider(),
		backupmanagerProv.NewMySQLJobFactsExtractor(mdb),
		metadb.MysqlCluster,
		cfg.Executer.ByClusterType(metadb.MysqlCluster))
	if err != nil {
		return nil, xerrors.Errorf("mysql executer: %w", err)
	}

	clickhouseExecuter, err := executerProv.NewExecuter(mdb,
		lg.WithName("clickhouse_executer"),
		deploy,
		hostPickerFromCtype(cfg.HostPicker, mdb, health, lg, metadb.ClickhouseCluster),
		backupmanagerProv.NewClickhouseS3DBBackupManager(s3Client, lg),
		nil,
		backupmanagerProv.NewClickhouseDeployArgsProvider(),
		backupmanagerProv.NewClickhouseJobFactsExtractor(mdb),
		metadb.ClickhouseCluster,
		cfg.Executer.ByClusterType(metadb.ClickhouseCluster))
	if err != nil {
		return nil, xerrors.Errorf("clickhouse executer: %w", err)
	}
	executers := map[metadb.ClusterType]executer.Executer{
		metadb.MongodbCluster:    mongodbExecuter,
		metadb.PostgresqlCluster: postgresqlExecuter,
		metadb.MysqlCluster:      mysqlExecuter,
		metadb.ClickhouseCluster: clickhouseExecuter,
	}

	execs := splitExecuters(executers)

	var pipelines []worker.Pipeline
	if cfg.Cycles.Planned.Enabled {
		pconf := cfg.Cycles.Planned
		delayFunc, err := delayer.NextFractionDelayerFunc(pconf.FractionDelayer)
		if err != nil {
			return nil, err
		}
		name := "PLANNED"
		pipelines = append(pipelines,
			worker.NewPipeline(
				name,
				queueProducerProv.NewQueueProducer(
					name,
					jobProducerProv.NewJobProducer(name, mdb, lg, mdb.PlannedBackup),
					pconf.QueueProducer,
					lg),
				queueHandlerProv.NewQueueHandler(
					name,
					jobHandlerProv.NewGenericJobHandler(name, mdb, lg, execs.Planned, delayFunc, pconf.JobHandler),
					pconf.QueueHandler,
					lg)))
	}
	if cfg.Cycles.Creating.Enabled {
		pconf := cfg.Cycles.Creating
		delayFunc, err := delayer.NextFractionDelayerFunc(pconf.FractionDelayer)
		if err != nil {
			return nil, err
		}
		name := "CREATING"
		pipelines = append(pipelines,
			worker.NewPipeline(
				name,
				queueProducerProv.NewQueueProducer(
					name,
					jobProducerProv.NewJobProducer(name, mdb, lg, mdb.CreatingBackup),
					pconf.QueueProducer,
					lg),
				queueHandlerProv.NewQueueHandler(
					name,
					jobHandlerProv.NewGenericJobHandler(name, mdb, lg, execs.Creating, delayFunc, pconf.JobHandler),
					pconf.QueueHandler,
					lg)))
	}
	if cfg.Cycles.Obsolete.Enabled {
		pconf := cfg.Cycles.Obsolete
		delayFunc, err := delayer.NextFractionDelayerFunc(pconf.FractionDelayer)
		if err != nil {
			return nil, err
		}
		name := "OBSOLETE"
		pipelines = append(pipelines,
			worker.NewPipeline(
				name,
				queueProducerProv.NewQueueProducer(
					name,
					jobProducerProv.NewJobProducer(name, mdb, lg, mdb.ObsoleteBackup),
					pconf.QueueProducer,
					lg),
				queueHandlerProv.NewQueueHandler(
					name,
					jobHandlerProv.NewGenericJobHandler(name, mdb, lg, execs.Obsolete, delayFunc, pconf.JobHandler),
					pconf.QueueHandler,
					lg)))
	}
	if cfg.Cycles.Deleting.Enabled {
		pconf := cfg.Cycles.Deleting
		delayFunc, err := delayer.NextFractionDelayerFunc(pconf.FractionDelayer)
		if err != nil {
			return nil, err
		}
		name := "DELETING"
		pipelines = append(pipelines,
			worker.NewPipeline(
				name,
				queueProducerProv.NewQueueProducer(
					name,
					jobProducerProv.NewJobProducer(name, mdb, lg, mdb.DeletingBackup),
					pconf.QueueProducer,
					lg),
				queueHandlerProv.NewQueueHandler(
					name,
					jobHandlerProv.NewGenericJobHandler(name, mdb, lg, execs.Deleting, delayFunc, pconf.JobHandler),
					pconf.QueueHandler,
					lg)))
	}
	return worker.NewWorker(mdb, pipelines, cfg.Worker, lg), nil
}

type execFuncs struct {
	Planned,
	Creating,
	Obsolete,
	Deleting map[metadb.ClusterType]executer.ExecFunc
}

func splitExecuters(executers map[metadb.ClusterType]executer.Executer) execFuncs {
	ef := execFuncs{
		Planned:  make(map[metadb.ClusterType]executer.ExecFunc),
		Creating: make(map[metadb.ClusterType]executer.ExecFunc),
		Obsolete: make(map[metadb.ClusterType]executer.ExecFunc),
		Deleting: make(map[metadb.ClusterType]executer.ExecFunc),
	}
	for clusterType, executerInst := range executers {
		ef.Planned[clusterType] = executerInst.StartCreation
		ef.Creating[clusterType] = executerInst.CompleteCreating
		ef.Obsolete[clusterType] = executerInst.StartDeletion
		ef.Deleting[clusterType] = executerInst.CompleteDeleting
	}
	return ef
}
