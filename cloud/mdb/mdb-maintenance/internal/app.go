package internal

import (
	"context"
	"io/ioutil"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmanagergrpc "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	holidayshttp "a.yandex-team.ru/cloud/mdb/internal/holidays/httpapi"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/memoized"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/weekends"
	"a.yandex-team.ru/cloud/mdb/internal/notifier"
	notifierhttp "a.yandex-team.ru/cloud/mdb/internal/notifier/http"
	notifiernop "a.yandex-team.ru/cloud/mdb/internal/notifier/nop"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/cms"
	mntcms "a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/cms/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/maintainer"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb"
	metadbpg "a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/stat"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const configFilename = "mdb-maintenance-sync.yaml"

type ResourceManagerConfig struct {
	Target       string                `json:"target" yaml:"target"`
	ClientConfig grpcutil.ClientConfig `json:"client_config" yaml:"client_config"`
}

type HolidaysConfig struct {
	Enabled    bool                `json:"enabled" yaml:"enabled"`
	Memoized   bool                `json:"memoized" yaml:"memoized"`
	HTTPClient holidayshttp.Config `json:"http_client" yaml:"http_client"`
}

type Config struct {
	CMS                grpc.Config           `json:"cms" yaml:"cms"`
	App                app.Config            `json:"app" yaml:"app"`
	TaskConfigsDirName string                `json:"task_configs_dir_name" yaml:"task_configs_dir_name"`
	MetaDB             pgutil.Config         `json:"metadb" yaml:"metadb"`
	WaitDBTimeout      time.Duration         `json:"wait_db_timeout" yaml:"wait_db_timeout"`
	WaitDBPeriod       time.Duration         `json:"wait_db_period" yaml:"wait_db_period"`
	Maintainer         maintainer.Config     `json:"maintainer" yaml:"maintainer"`
	Notifier           notifierhttp.Config   `json:"notifier" yaml:"notifier"`
	ResourceManager    ResourceManagerConfig `json:"resource_manager" yaml:"resource_manager"`
	Holidays           HolidaysConfig        `json:"holidays" yaml:"holidays"`
}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	return Config{
		App:                app.DefaultConfig(),
		TaskConfigsDirName: "configs",
		MetaDB: pgutil.Config{
			Addrs: []string{"localhost:6432"},
			DB:    "dbaas_metadb",
			User:  "mdb_maintenance",
		},
		WaitDBTimeout: 60 * time.Second,
		WaitDBPeriod:  3 * time.Second,
		Maintainer:    maintainer.DefaultConfig(),
		Notifier:      notifierhttp.DefaultConfig(),
		Holidays: HolidaysConfig{
			Enabled:    false,
			Memoized:   true,
			HTTPClient: holidayshttp.DefaultConfig(),
		},
	}
}

type MaintenanceApp struct {
	*app.App
	Config  Config
	configs []models.MaintenanceTaskConfig
	MDB     metadb.MetaDB
	Mnt     *maintainer.Maintainer
	stat    stat.MaintenanceStat
}

func NewApp(baseApp *app.App, config Config, sender notifier.API, resClient resmanager.Client) (*MaintenanceApp, error) {
	ctx := context.Background()

	mdb, err := metadbpg.New(config.MetaDB, baseApp.L())
	if err != nil {
		return nil, err
	}
	err = ready.WaitWithTimeout(
		ctx,
		config.WaitDBTimeout,
		mdb,
		&ready.DefaultErrorTester{Name: "maintenance sync MDB", L: baseApp.L()},
		config.WaitDBPeriod)
	if err != nil {
		return nil, err
	}
	var mntCMS cms.CMS
	if config.CMS.Host == "" {
		baseApp.L().Info("cms host is not set, cms not enabled")
	} else {
		creds := &grpcutil.PerRPCCredentialsStatic{}
		insecure := true
		if !config.App.ServiceAccount.Empty() {
			insecure = false
			client, err := iamgrpc.NewTokenServiceClient(
				baseApp.ShutdownContext(),
				config.App.Environment.Services.Iam.V1.TokenService.Endpoint,
				config.App.AppName,
				grpcutil.DefaultClientConfig(),
				&grpcutil.PerRPCCredentialsStatic{},
				baseApp.L(),
			)
			if err != nil {
				return nil, err
			}
			_ = client.ServiceAccountCredentials(iam.ServiceAccount{
				ID:    config.App.ServiceAccount.ID,
				KeyID: config.App.ServiceAccount.KeyID.Unmask(),
				Token: []byte(config.App.ServiceAccount.PrivateKey.Unmask()),
			})
		}
		instanceClient, err := grpc.NewFromConfig(baseApp.ShutdownContext(), grpc.Config{
			Host:      config.CMS.Host,
			Transport: grpcutil.ClientConfig{Security: grpcutil.SecurityConfig{Insecure: insecure}}},
			"cms-test",
			baseApp.L(),
			creds,
		)
		if err != nil {
			return nil, err
		}
		mntCMS = mntcms.NewGRPCCMS(instanceClient, mdb)
		baseApp.L().Info("cms enabled")
	}

	mntStat := stat.MaintenanceStat{
		ConfigCount:                baseApp.Metrics().Counter("mdb_maintenance_config_count"),
		ConfigErrorCount:           baseApp.Metrics().Counter("mdb_maintenance_config_error_count"),
		MaintenanceDuration:        baseApp.Metrics().Timer("mdb_maintenance_duration"),
		ConfigPlanClusterCount:     baseApp.Metrics().CounterVec("mdb_maintenance_config_plan_cluster_count", []string{"config_id", "action"}),
		ConfigSelectedClusterCount: baseApp.Metrics().CounterVec("mdb_maintenance_config_selected_cluster_count", []string{"config_id"}),
	}

	if sender == nil {
		if config.Notifier.Endpoint == "" {
			sender = notifiernop.NewAPI()
		} else {
			sender, err = notifierhttp.NewAPI(config.Notifier, baseApp.L())
			if err != nil {
				return nil, err
			}
		}
	}
	err = sender.IsReady(context.Background())
	if err != nil {
		return nil, err
	}

	if resClient == nil && config.Maintainer.NotifyUserDirectly {
		resClient, err = resmanagergrpc.NewClient(
			ctx,
			config.ResourceManager.Target,
			"mdb-maintenance",
			config.ResourceManager.ClientConfig,
			baseApp.ServiceAccountCredentials(),
			baseApp.L(),
		)
		if err != nil {
			return nil, err
		}
	}
	var cal holidays.Calendar
	if config.Holidays.Enabled {
		calClient, err := holidayshttp.New(config.Holidays.HTTPClient, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("create holidays client: %w", err)
		}
		cal = calClient
		if config.Holidays.Memoized {
			cal = memoized.New(cal)
		}
	} else {
		cal = &weekends.Calendar{}
	}
	a := &MaintenanceApp{
		App:    baseApp,
		Config: config,
		MDB:    mdb,
		Mnt:    maintainer.New(mntCMS, baseApp.L(), mdb, config.Maintainer, mntStat, sender, resClient, cal),
		stat:   mntStat,
	}
	return a, nil
}

func (a *MaintenanceApp) LoadConfig(configName string) (models.MaintenanceTaskConfig, error) {
	configAbsPath := config.JoinWithConfigPath(filepath.Join(a.Config.TaskConfigsDirName, configName))
	cfg := models.DefaultTaskConfig()
	if err := config.LoadFromAbsolutePath(configAbsPath, &cfg); err != nil {
		return cfg, err
	}
	cfg.ID = strings.TrimSuffix(configName, filepath.Ext(configName))
	return cfg, nil
}

func (a *MaintenanceApp) loadConfigs() error {
	files, err := ioutil.ReadDir(config.JoinWithConfigPath(a.Config.TaskConfigsDirName))
	if err != nil {
		return err
	}

	for _, f := range files {
		if f.IsDir() {
			continue
		}

		cfg, err := a.LoadConfig(f.Name())
		if err != nil {
			a.L().Errorf("Failed to load of maintenance task configs: %v", err)
			a.stat.ConfigErrorCount.Inc()
			continue
		}
		a.configs = append(a.configs, cfg)
		a.stat.ConfigCount.Inc()
	}
	a.L().Infof("Loaded %v maintenance task configs", len(a.configs))
	return nil

}

func (a *MaintenanceApp) Sync() {
	started := time.Now()
	if err := a.loadConfigs(); err != nil {
		a.L().Fatalf("loading config error: %v", err)
	}
	ctx := a.ShutdownContext()

	priorities := map[string]int{}
	for _, cfg := range a.configs {
		priorities[cfg.ID] = cfg.Priority
	}
	sort.Slice(a.configs, func(i, j int) bool {
		return priorities[a.configs[j].ID] < priorities[a.configs[i].ID]
	})

	for _, cfg := range a.configs {
		if err := a.MaintainConfig(ctx, cfg, priorities); err != nil {
			sentry.GlobalClient().CaptureError(ctx, err, map[string]string{"config": cfg.ID})
			a.L().Fatalf("config %+v main failed: %s", cfg, err)
		}
	}
	a.stat.MaintenanceDuration.RecordDuration(time.Since(started))
	a.L().Info("Maintenance materialization is completed")
}

func (a *MaintenanceApp) MaintainConfig(ctx context.Context, taskConfig models.MaintenanceTaskConfig, priorities map[string]int) error {
	ctx = ctxlog.WithFields(ctx, log.String("config_id", taskConfig.ID))
	return a.Mnt.Maintain(ctx, taskConfig, priorities)
}

func Run() {
	cfg := DefaultConfig()
	opts := app.DefaultToolOptions(&cfg, configFilename)
	opts = append(opts, app.WithMetrics())
	opts = append(opts, app.WithMetricsSendOnShutdown())
	opts = append(opts, app.WithSentry())
	baseApp, err := app.New(opts...)
	if err != nil {
		panic(err)
	}
	l := baseApp.L()
	if !cfg.MetaDB.Password.FromEnv("METADB_PASSWORD") {
		l.Info("METADB_PASSWORD is empty")
	}
	if !cfg.App.ServiceAccount.FromEnv("MAINTENANCE") {
		l.Info("MAINTENANCE_PRIVATE_KEY is empty")
	}
	a, err := NewApp(baseApp, cfg, nil, nil)
	if err != nil {
		sentry.GlobalClient().CaptureErrorAndWait(baseApp.ShutdownContext(), err, map[string]string{})
		baseApp.L().Fatalf("app creation error: %v", err)
	}
	defer a.Shutdown()
	a.L().Info("maintenance tool is created")
	a.Sync()
}
