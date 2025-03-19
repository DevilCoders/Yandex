package cli

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/pg"
	backupmanagerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager/provider"
	executerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/executer/provider"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/hostpicker"
	hostpickerConfig "a.yandex-team.ru/cloud/mdb/backup/worker/internal/hostpicker/provider"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer/clickhouse"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer/mongodb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer/mysql"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer/postgresql"
	wrkrApp "a.yandex-team.ru/cloud/mdb/backup/worker/pkg/app"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapirest "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	s3Prov "a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthswagger "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	AppName = "cli"
)

type RollerConfig struct {
	State              string                `json:"state" yaml:"state"`
	Timeout            encodingutil.Duration `json:"timeout" yaml:"timeout"`
	StatusCheckerRetry retry.Config          `json:"status_checker_retry" yaml:"status_checker_retry"`
}

type Config struct {
	App         app.Config                            `json:"app" yaml:"app"`
	Deploy      deployapirest.Config                  `json:"deploy" yaml:"deploy"`
	Health      healthswagger.Config                  `json:"health" yaml:"health"`
	S3Client    s3Prov.Config                         `json:"s3" yaml:"s3"`
	HostPicker  hostpickerConfig.HostPickersMapConfig `json:"host_picker" yaml:"host_picker"`
	Metadb      pgutil.Config                         `json:"metadb" yaml:"metadb"`
	InitTimeout encodingutil.Duration                 `json:"init_timeout" yaml:"init_timeout"`
	Roller      RollerConfig                          `json:"roller" yaml:"roller"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	return Config{
		App: app.DefaultConfig(),
		Metadb: pgutil.Config{
			User: "backup_cli",
			DB:   mdbpg.DBName,
		},
		Deploy:      deployapirest.DefaultConfig(),
		Health:      healthswagger.DefaultConfig(),
		S3Client:    s3Prov.Config{},
		HostPicker:  hostpickerConfig.DefaultConfig(),
		InitTimeout: encodingutil.FromDuration(10 * time.Second),
		Roller: RollerConfig{
			State:              "components.dbaas-operations.metadata",
			Timeout:            encodingutil.FromDuration(5 * time.Minute),
			StatusCheckerRetry: retry.Config{InitialInterval: 15 * time.Second, MaxElapsedTime: 6 * time.Minute, MaxInterval: time.Minute},
		},
	}
}

func depsConfigFromCli(cfg Config) wrkrApp.DepsConfig {
	return wrkrApp.DepsConfig{
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
	mdb      metadb.MetaDB
	deploy   deployapi.Client
	health   client.MDBHealthClient
	s3Client s3.Client
	L        log.Logger
}

type Cli struct {
	cfg      Config
	mdb      metadb.MetaDB
	deploy   deployapi.Client
	health   client.MDBHealthClient
	s3Client s3.Client
	idGen    generator.IDGenerator
	L        log.Logger
}

// NewAppFromConfig builds cli from config
func NewAppFromConfig(ctx context.Context) (*Cli, log.Logger, error) {
	cfg := DefaultConfig()
	baseApp, err := app.New(append(app.DefaultToolOptions(&cfg, fmt.Sprintf("%s.yaml", AppName)), app.WithNoFlagParse())...)
	if err != nil {
		return nil, nil, xerrors.Errorf("cannot make base app, %w", err)
	}
	lg := baseApp.L()

	deps, err := wrkrApp.NewExtDepsFromConfig(baseApp, depsConfigFromCli(cfg))
	if err != nil {
		return nil, nil, err
	}

	cliApp := NewAppFromExtDeps(cfg, deps.Mdb, deps.Deploy, deps.Health, deps.S3Client, &generator.BackupIDGenerator{}, lg)
	if err = ready.Wait(ctx, cliApp.mdb, &ready.DefaultErrorTester{Name: "metadb", L: lg}, cfg.InitTimeout.Duration); err != nil {
		return nil, nil, xerrors.Errorf("failed to wait backend: %w", err)
	}

	return cliApp, lg, nil
}

func NewAppFromExtDeps(cfg Config, mdb metadb.MetaDB, deploy deployapi.Client, health client.MDBHealthClient, s3Client s3.Client, idGen generator.IDGenerator, lg log.Logger) *Cli {
	return &Cli{
		cfg:      cfg,
		mdb:      mdb,
		deploy:   deploy,
		health:   health,
		s3Client: s3Client,
		idGen:    idGen,
		L:        lg,
	}
}

func (c *Cli) IsReady(ctx context.Context) error {
	return c.mdb.IsReady(ctx)
}

func (c *Cli) SetBackupServiceEnabled(ctx context.Context, cid string, shouldBeEnabled bool) error {
	txCtx, err := c.mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return err
	}

	defer c.mdb.Rollback(txCtx)

	bsEnabled, err := c.mdb.BackupServiceEnabled(txCtx, cid)
	if err != nil {
		return err
	}

	if bsEnabled == shouldBeEnabled {
		c.L.Debugf("Backup-service is already enabled for cluster %q", cid)
		return nil
	}

	running, err := c.mdb.ClusterStatusIsIn(txCtx, cid, []metadb.ClusterStatus{metadb.ClusterStatusRunning})
	if err != nil {
		return err
	}
	if !running {
		return xerrors.Errorf("cluster is not in RUNNING state")
	}

	c.L.Debugf("Switching backup-service usage for %q cluster to %t", cid, shouldBeEnabled)
	if err := c.mdb.SetBackupServiceEnabled(txCtx, cid, shouldBeEnabled); err != nil {
		return err
	}
	return c.mdb.Commit(txCtx)
}

func (c *Cli) RollMetadata(ctx context.Context, cid string, ignoreHealthCheck bool) error {
	hosts, err := c.mdb.ListHosts(ctx, cid, optional.String{}, optional.String{})
	if err != nil {
		return err
	}
	fqdns := metadb.FqdnsFromHosts(hosts)
	c.L.Debugf("Fetched cluster hosts: %+v", fqdns)
	if err := hostpicker.AssertHostsHealth(ctx, c.health, fqdns); err != nil {
		c.L.Error(err.Error())
		if !ignoreHealthCheck {
			return err
		}
	}

	c.L.Debugf("Rolling metadata on %q cluster hosts", cid)

	cfg := c.cfg.Roller
	shipmentID, _, err := executerProv.CreateShipment(ctx, []string{cfg.State, "concurrent=True", "sync_mods=states,modules"},
		fqdns, cfg.Timeout, c.deploy)
	if err != nil {
		return err
	}

	if err := retry.New(cfg.StatusCheckerRetry).RetryWithLog(
		ctx,
		func() error {
			if err := executerProv.AssertShipmentCompleted(ctx, shipmentID, c.deploy); err != nil {
				if metadb.FromError(err).Temporary() {
					return err
				}
				return retry.Permanent(err)
			}
			return nil
		},
		"get shipment status",
		c.L,
	); err != nil {
		return xerrors.Errorf("failed to complete shipment %q: %w", shipmentID, err)
	}

	c.L.Infof("Metadata rolling shipment is completed, cluster hosts: %s", strings.Join(fqdns, ", "))

	return nil
}

func (c *Cli) ImportBackups(ctx context.Context, cid string, skipSchedDateDups, completeFailed, dryrun bool) (importer.ImportStats, error) {
	cluster, err := c.mdb.Cluster(ctx, cid)
	if err != nil {
		return importer.ImportStats{}, err
	}

	var stats importer.ImportStats
	switch cluster.Type {
	case metadb.MongodbCluster:
		imp := mongodb.NewImporter(
			backupmanagerProv.NewMongoDBBackupManager(c.s3Client, c.L),
			c.mdb, c.s3Client, c.idGen, c.L)
		if err = ready.Wait(ctx, imp, &ready.DefaultErrorTester{Name: "metadb", L: c.L}, c.cfg.InitTimeout.Duration); err != nil {
			return importer.ImportStats{}, xerrors.Errorf("failed to wait backend: %w", err)
		}

		stats, err = imp.Import(ctx, cluster.ClusterID, skipSchedDateDups, dryrun)
	case metadb.PostgresqlCluster:
		imp := postgresql.NewImporter(
			backupmanagerProv.NewPostgreSQLBackupManager(c.s3Client, c.L),
			c.mdb, c.s3Client, c.idGen, c.L)
		if err = ready.Wait(ctx, imp, &ready.DefaultErrorTester{Name: "metadb", L: c.L}, c.cfg.InitTimeout.Duration); err != nil {
			return importer.ImportStats{}, xerrors.Errorf("failed to wait backend: %w", err)
		}

		stats, err = imp.Import(ctx, cluster.ClusterID, skipSchedDateDups, completeFailed, dryrun)
	case metadb.MysqlCluster:
		imp := mysql.NewImporter(
			backupmanagerProv.NewMySQLBackupManager(c.s3Client, c.L),
			c.mdb, c.s3Client, c.idGen, c.L)
		if err = ready.Wait(ctx, imp, &ready.DefaultErrorTester{Name: "metadb", L: c.L}, c.cfg.InitTimeout.Duration); err != nil {
			return importer.ImportStats{}, xerrors.Errorf("failed to wait backend: %w", err)
		}

		stats, err = imp.Import(ctx, cluster.ClusterID, skipSchedDateDups, dryrun)
	case metadb.ClickhouseCluster:
		imp := clickhouse.NewImporter(
			backupmanagerProv.NewClickhouseS3DBBackupManager(c.s3Client, c.L),
			c.mdb, c.s3Client, c.idGen, c.L)
		if err = ready.Wait(ctx, imp, &ready.DefaultErrorTester{Name: "metadb", L: c.L}, c.cfg.InitTimeout.Duration); err != nil {
			return importer.ImportStats{}, xerrors.Errorf("failed to wait backend: %w", err)
		}

		stats, err = imp.Import(ctx, cluster.ClusterID, skipSchedDateDups, dryrun)

	default:
		return importer.ImportStats{}, xerrors.Errorf("unsupported cluster_type to import backups: %s", cluster.ClusterID)
	}

	if updErr := c.updateImportHistory(ctx, cid, err); updErr != nil {
		return importer.ImportStats{}, xerrors.Errorf("update import history (cid: %s, err: %v): %w", cid, err, updErr)
	}

	return stats, err
}

func (c *Cli) updateImportHistory(ctx context.Context, cid string, importErr error) error {
	txCtx, err := c.mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return err
	}

	defer c.mdb.Rollback(txCtx)

	err = c.mdb.UpdateImportHistory(txCtx, cid, importErr, time.Now())
	if err != nil {
		return err
	}

	return c.mdb.Commit(txCtx)
}

func (c *Cli) BatchImportBackups(ctx context.Context, clusterTypes []metadb.ClusterType, batchSize int,
	interval time.Duration, skipSchedDateDups, completeFailed, dryrun bool) (importer.ImportStats, error) {
	clustersForImport, err := c.mdb.GetClustersForImport(ctx, batchSize, clusterTypes, interval)
	if err != nil {
		return importer.ImportStats{}, xerrors.Errorf("get clusters for import: %w", err)
	}

	allStats := importer.ImportStats{}

	for _, cid := range clustersForImport {
		stats, err := c.ImportBackups(ctx, cid, skipSchedDateDups, completeFailed, dryrun)
		if err != nil {
			c.L.Errorf("import backups for cluster %s: %v", cid, err)
			continue
		}

		allStats.Append(stats)
	}

	return allStats, nil
}
