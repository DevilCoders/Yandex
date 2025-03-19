package internal

import (
	"io/ioutil"
	"net/http"
	"os"
	"time"

	"github.com/go-openapi/runtime/middleware"
	goredis "github.com/go-redis/redis/v8"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/swagger"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/leaderelection"
	rediscachetierle "a.yandex-team.ru/cloud/mdb/internal/leaderelection/rediscachetier"
	redsyncle "a.yandex-team.ru/cloud/mdb/internal/leaderelection/redsync"
	"a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/prometheus"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore/redis"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore"
	_ "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore/pg"
	"a.yandex-team.ru/library/go/core/log"
)

// App main application object - handles setup and teardown
type App struct {
	*swagger.App

	GW  *core.GeneralWard
	Cfg Config
}

type Config struct {
	App     app.Config            `json:"app" yaml:"app"`
	Swagger swagger.SwaggerConfig `json:"swagger" yaml:"swagger"`

	GW           core.GWConfig      `json:"gw" yaml:"gw"`
	DataStore    string             `json:"datastore" yaml:"datastore"`
	HealthStore  healthstore.Config `json:"healthstore" yaml:"healthstore"`
	SecretsStore string             `json:"secretsstore" yaml:"secretsstore"`

	ClickHouse    chutil.Config `json:"clickhouse" yaml:"clickhouse"`
	LeaderElector string        `json:"leaderelector" yaml:"leaderelector"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App:           app.DefaultConfig(),
		Swagger:       swagger.DefaultSwaggerConfig(),
		GW:            core.DefaultConfig(),
		DataStore:     "redis",
		HealthStore:   healthstore.DefaultConfig(),
		SecretsStore:  "postgresql",
		ClickHouse:    chutil.Config{},
		LeaderElector: "rediscachetier",
	}

	cfg.App.Tracing.ServiceName = "mdb-health"
	return cfg
}

const (
	configName       = "mdbh.yaml"
	configMetaDBName = "mdbhsspg.yaml"

	envClusterKeyOverride = "MDBH_CLUSTERKEYOVERRIDE"

	leaderElectionRedsync        = "redsync"
	leaderElectionRedisCacheTier = "rediscachetier"
)

func NewApp(mdwCtx *middleware.Context) *App {
	cfg := DefaultConfig()
	baseApp, err := swagger.New(mdwCtx, app.DefaultServiceOptions(&cfg, configName)...)
	if err != nil {
		panic(err)
	}

	// Load datastore backend
	ds, err := datastore.Open(cfg.DataStore, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("Failed to open %q datastore: %s", cfg.DataStore, err)
	}

	if err = ready.WaitWithTimeout(baseApp.ShutdownContext(), 10*time.Second, ds, &ready.DefaultErrorTester{Name: "datastore", L: baseApp.L()}, time.Second); err != nil {
		baseApp.L().Fatal("can not init datastore", log.Error(err))
	}

	// Load healthstore
	healthstoreBackends := []healthstore.Backend{ds}

	if len(cfg.ClickHouse.Addrs) > 0 {
		chstore, err := clickhouse.New(baseApp.L(), cfg.ClickHouse)
		if err != nil {
			baseApp.L().Fatalf("Failed to open clickhouse healthstore: %s", err)
		}

		if err = ready.WaitWithTimeout(baseApp.ShutdownContext(), 10*time.Second, chstore, &ready.DefaultErrorTester{Name: "ch healthstore", L: baseApp.L()}, time.Second); err != nil {
			baseApp.L().Fatal("can not init ch healthstore", log.Error(err))
		}

		healthstoreBackends = append(healthstoreBackends, chstore)
	}

	hs := healthstore.NewStore(healthstoreBackends, cfg.HealthStore, baseApp.L())
	go hs.Run(baseApp.ShutdownContext())

	// Load secretsstore backend
	ss, err := secretsstore.Open(cfg.SecretsStore, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("Failed to open %q secretsstore: %s", cfg.SecretsStore, err)
	}

	var keyFile []byte
	if path, ok := os.LookupEnv(envClusterKeyOverride); ok {
		baseApp.L().Warn("!!!!!!!!!!!!!!!!!! LOADING CLUSTER KEY OVERRIDE !!!!!!!!!!!!!!!!!!")
		keyFile, err = ioutil.ReadFile(path)
		if err != nil {
			baseApp.L().Fatalf("Failed to open cluster key override %q: %s", path, err)
		}

		baseApp.L().Warnf(
			"!!!!!!!!!!!!!!!!!! LOADED CLUSTER KEY OVERRIDE FROM PATH %q !!!!!!!!!!!!!!!!!!",
			path,
		)
	}

	pgConf := pg.LoadConfig(baseApp.L(), configMetaDBName)
	mdb, err := pg.New(pgConf, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("Failed to prepare MetaDB endpoint: %s", err)
	}

	var leaderElector leaderelection.LeaderElector
	switch cfg.LeaderElector {
	case leaderElectionRedsync:
		redisConf, err := redis.LoadConfig(baseApp.L())
		if err != nil {
			baseApp.L().Fatalf("load redis config: %s", err)
		}
		redisClientOptions := redis.UniversalClientOptions(redisConf)
		redisUniversal := goredis.NewUniversalClient(redisClientOptions)
		leConf := redsyncle.DefaultConfig()
		leConf.AppName = cfg.App.AppName
		leConf.LeaderTTL = cfg.GW.LeadTimeout
		instanceID, err := os.Hostname()
		if err != nil {
			baseApp.L().Fatalf("get hostname for leader election: %s", err)
		}
		leConf.InstanceID = instanceID
		leaderElector = redsyncle.New(baseApp.ShutdownContext(), redisUniversal, leConf, baseApp.L())
	case leaderElectionRedisCacheTier:
		leConf := rediscachetierle.DefaultConfig()
		leConf.LeaderTTL = cfg.GW.LeadTimeout
		leaderElector = rediscachetierle.New(baseApp.ShutdownContext(), leConf, baseApp.L())
	default:
		baseApp.L().Fatalf("Unknown leader elector type: %s", cfg.LeaderElector)
	}

	gw := core.NewGeneralWard(baseApp.ShutdownContext(), baseApp.L(), cfg.GW, ds, hs, ss, mdb, keyFile, true, leaderElector)

	gw.RegisterReadyChecker(baseApp.ReadyCheckAggregator())

	a := &App{
		App: baseApp,
		Cfg: cfg,
		GW:  gw,
	}
	return a
}

// GeneralWard accessor
func (app *App) GeneralWard() *core.GeneralWard {
	return app.GW
}

// SetupGlobalMiddleware setups global middleware
func (app *App) SetupGlobalMiddleware(next http.Handler) http.Handler {
	return httputil.RequestIDMiddleware(
		httputil.LoggingMiddleware(
			httputil.RequestBodySavingMiddleware(
				prometheus.Middleware(
					httputil.TracingSwaggerMiddleware(next, app.Cfg.App.Tracing.ServiceName, app.MiddlewareContext()),
					app.L(),
				),
				app.L(),
			),
			app.Cfg.Swagger.Logging,
			app.L(),
		),
	)
}
