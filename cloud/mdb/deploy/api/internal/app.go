package internal

import (
	"net/http"
	"os"

	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/core"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	_ "a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb/pg" // Load PostgreSQL backend
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/swagger"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
)

// App main application object - handles setup and teardown
type App struct {
	*swagger.App

	cfg Config
	srv *core.Service
}

type Config struct {
	App       app.Config            `json:"app" yaml:"app"`
	Swagger   swagger.SwaggerConfig `json:"swagger" yaml:"swagger"`
	DeployAPI core.Config           `json:"deploy_api" yaml:"deploy_api"`
	DB        string                `json:"db" yaml:"db"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App:       app.DefaultConfig(),
		Swagger:   swagger.DefaultSwaggerConfig(),
		DeployAPI: core.DefaultConfig(),
		DB:        "postgresql",
	}
	cfg.App.Tracing.ServiceName = "mdb-deploy-api"
	return cfg
}

const (
	configName = "mdb-deploy-api.yaml"
)

func NewApp(mdwCtx *middleware.Context) *App {
	cfg := DefaultConfig()
	baseApp, err := swagger.New(mdwCtx, app.DefaultServiceOptions(&cfg, configName)...)
	if err != nil {
		panic(err)
	}

	if !cfg.DeployAPI.SaltAPI.Auth.Password.FromEnv("DEPLOY_SALT_API_PASSWORD") {
		baseApp.L().Warn("salt api password is empty")
	}

	if cfg.DeployAPI.MasterCheckerName == "" {
		baseApp.L().Info("Master checker name is not set in config. Retrieving it from the machine.")
		if cfg.DeployAPI.MasterCheckerName, err = os.Hostname(); err != nil {
			baseApp.L().Fatalf("failed to get hostname: %s", err)
		}
	}

	jrBlacklist := core.DefaultJobResultBlacklist()
	if err = config.Load(cfg.DeployAPI.JobResultBlacklistPath, &jrBlacklist); err != nil {
		baseApp.L().Errorf("failed to load job results blacklist, using defaults: %s", err)
		jrBlacklist = core.DefaultJobResultBlacklist()
	}

	baseApp.L().Infof("Using job results blacklist: %+v", jrBlacklist)

	// Load db backend
	b, err := deploydb.Open(cfg.DB, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("Failed to open '%s' db: %s", cfg.DB, err)
	}

	srv, err := core.NewService(baseApp.ShutdownContext(), b, cfg.DeployAPI, jrBlacklist, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("Failed to create service: %s", err)
	}

	a := &App{
		App: baseApp,
		cfg: cfg,
		srv: srv,
	}
	return a
}

// SetupGlobalMiddleware setups global middleware
func (app *App) SetupGlobalMiddleware(next http.Handler) http.Handler {
	return httputil.ChainStandardSwaggerMiddleware(
		next,
		app.cfg.App.Tracing.ServiceName,
		app.MiddlewareContext(),
		app.cfg.Swagger.Logging,
		app.L(),
	)
}

// Service provides access to service
func (app *App) Service() *core.Service {
	return app.srv
}
