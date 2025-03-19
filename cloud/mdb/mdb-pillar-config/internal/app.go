package internal

import (
	"net"
	"net/http"
	"os"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth/nop"
	authprovider "a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb"
	pgmdb "a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

const (
	configName = "mdb-pillar-config.yaml"
)

// Config describes base config
type Config struct {
	App    app.Config           `json:"app" yaml:"app"`
	API    api.Config           `json:"api" yaml:"api"`
	HTTP   httputil.ServeConfig `json:"http" yaml:"http"`
	MetaDB pgutil.Config        `json:"metadb" yaml:"metadb"`
	// TODO: remove after https://st.yandex-team.ru/ORION-109
	DisableAuth bool `json:"disable_auth" yaml:"disable_auth"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App:    app.DefaultConfig(),
		API:    api.DefaultConfig(),
		HTTP:   httputil.DefaultServeConfig(),
		MetaDB: pgmdb.DefaultConfig(),
	}

	cfg.App.Logging.File = "/var/log/mdb-pillar-config/mdb-pillar-config.log"
	cfg.App.Tracing.ServiceName = "mdb-pillar-config"

	return cfg
}

type AppComponents struct {
	MetaDB        metadb.MetaDB
	Authenticator auth.Authenticator
}

type App struct {
	*app.App
	AppComponents

	Config       Config
	httpServer   *http.Server
	grpcListener net.Listener
}

func Run(components AppComponents) {
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(&cfg, configName)...)
	if err != nil {
		panic(err)
	}

	if !cfg.MetaDB.Password.FromEnv("METADB_PASSWORD") {
		baseApp.L().Info("METADB_PASSWORD is empty")
	}

	if cfg.DisableAuth {
		components.Authenticator = &nop.Authenticator{}
	}

	a, err := NewApp(baseApp, cfg, components)
	if err != nil {
		baseApp.L().Errorf("failed to create app: %+v", err)
		os.Exit(1)
	}

	a.WaitForStop()
}

func NewApp(baseApp *app.App, cfg Config, components AppComponents) (*App, error) {
	vctx := valid.NewValidationCtx()
	if verr := valid.Struct(vctx, cfg); verr != nil {
		return nil, xerrors.Errorf("failed to validate config: %w", verr)
	}

	// Initialize metadb client
	if components.MetaDB == nil {
		mdb, err := pgmdb.New(cfg.MetaDB, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize metadb backend: %w", err)
		}

		components.MetaDB = mdb
	}

	// Initialize authenticator
	if components.Authenticator == nil {
		components.Authenticator = authprovider.NewMetaDBAuthenticator(components.MetaDB)
	}

	a := &App{
		App:           baseApp,
		AppComponents: components,
		Config:        cfg,
	}

	r := chi.NewRouter()
	r.Use(func(next http.Handler) http.Handler {
		return httputil.ChainStandardMiddleware(api.AuthFromRequest(next), cfg.App.Tracing.ServiceName, cfg.HTTP.Logging, baseApp.L())
	})

	// Register handlers
	configResponder := api.ConfigResponder{
		MetaDB:        components.MetaDB,
		Authenticator: components.Authenticator,
		L:             baseApp.L(),
		Config:        cfg.API,
	}
	if err := configResponder.RegisterConfigHandlers(r); err != nil {
		return nil, xerrors.Errorf("register dynamic handlers: %w", err)
	}

	a.httpServer = &http.Server{Addr: cfg.HTTP.Addr, Handler: r}
	if err := httputil.Serve(a.httpServer, baseApp.L()); err != nil {
		return nil, err
	}

	return a, nil
}

func (a *App) WaitForStop() {
	a.App.WaitForStop()
	if err := httputil.Shutdown(a.httpServer, a.Config.HTTP.ShutdownTimeout); err != nil {
		a.L().Error("failed to shutdown HTTP server", log.Error(err))
	}
}
