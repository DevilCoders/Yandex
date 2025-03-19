package webservice

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"time"

	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication/tvm"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/swagger"
	"a.yandex-team.ru/cloud/mdb/internal/dbm/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// SwaggerApp main application object - handles setup and teardown
type SwaggerApp struct {
	*swagger.App

	srv *Service
	cfg internal.Config
}

type ExitStatus int

const (
	configErrorExit ExitStatus = iota + 1
)

func exit(status ExitStatus, msg string) {
	fmt.Println(msg)
	os.Exit(int(status))
}

func NewSwaggerApp(mdwCtx *middleware.Context) *SwaggerApp {
	cfg := internal.DefaultConfig()
	baseApp, err := swagger.New(mdwCtx, app.DefaultServiceOptions(&cfg, fmt.Sprintf("%s.yaml", internal.AppCfgName))...)
	if err != nil {
		msg := fmt.Sprintf("Failed to configure swagger: %s", err)
		exit(configErrorExit, msg)
	}

	// Load db backend
	db, err := pg.NewCMSDBWithConfigLoad(baseApp.L())
	if err != nil {
		msg := fmt.Sprintf("Failed to create cmsdb: %s", err)
		baseApp.L().Fatalf(msg)
		exit(configErrorExit, msg)
	}

	auth, err := tvm.NewFromConfig(cfg.Auth.Tvm, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("Failed to create tvm auth: %s", err)
	}

	// dbm create
	dbm, err := restapi.New(cfg.Dbm, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("Failed to create dbm: %s", err)
	}

	srv, err := NewService(
		db,
		dbm,
		baseApp.L(),
		auth,
		cfg.Cms)
	if err != nil {
		msg := fmt.Sprintf("Failed to create service: %s", err)
		baseApp.L().Fatalf(msg)
		exit(configErrorExit, msg)
	}
	ctx := baseApp.ShutdownContext()
	awaitCtx, cancel := context.WithTimeout(ctx, 10*time.Second)
	defer cancel()
	waitForInterval := time.Second
	err = ready.Wait(awaitCtx, srv, &ready.DefaultErrorTester{Name: "cms service", L: baseApp.L()}, waitForInterval)
	if err != nil {
		msg := fmt.Sprintf("Service initialization timeout: %s", err)
		baseApp.L().Fatalf(msg)
		exit(configErrorExit, msg)
	}

	go srv.RunBackgroundLoop(ctx)

	return &SwaggerApp{
		App: baseApp,
		cfg: cfg,
		srv: srv,
	}
}

func CustomSwaggerApp(dbHostPorts string, auth authentication.Authenticator) (*SwaggerApp, cmsdb.Client, log.Logger) {
	cfg := internal.DefaultConfig()
	opts := []app.AppOption{
		app.WithConfig(&cfg),
		app.WithLoggerConstructor(app.DefaultServiceLoggerConstructor()),
	}
	baseApp, err := swagger.New(nil, opts...)

	if err != nil {
		msg := fmt.Sprintf("Failed to create swagger: %s", err)
		baseApp.L().Fatalf(msg)
		exit(configErrorExit, msg)
	}

	// Load db backend
	var defaultDB cmsdb.Client
	pgcfg := pg.DefaultConfig()
	pgcfg.Addrs = []string{dbHostPorts}
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	defaultDB, err = pg.New(pgcfg, logger)
	if err != nil {
		exit(configErrorExit, "not ready db")
	}

	awaitCtx, cancel := context.WithTimeout(baseApp.ShutdownContext(), 30*time.Second)
	defer cancel()
	err = ready.Wait(awaitCtx, defaultDB, &ready.DefaultErrorTester{Name: "cms database", L: logger}, time.Second)
	if err != nil {
		exit(configErrorExit, "not ready backend")
	}

	dbm, err := restapi.New(cfg.Dbm, baseApp.L())
	if err != nil {
		exit(configErrorExit, "dbm not configured")
	}

	srv, err := NewService(defaultDB, dbm, baseApp.L(), auth, cfg.Cms)
	if err != nil {
		exit(configErrorExit, "cannot make service")
	}

	return &SwaggerApp{
		App: baseApp,
		cfg: cfg,
		srv: srv,
	}, defaultDB, logger
}

// SetupGlobalMiddleware setups global middleware
func (app *SwaggerApp) SetupGlobalMiddleware(next http.Handler) http.Handler {
	// TODO: configure tracing
	return httputil.ChainStandardMiddleware(next, fmt.Sprintf("%s-api", app.cfg.App.AppName), app.cfg.Swagger.Logging, app.L())
}

// Service provides access to service
func (app *SwaggerApp) Service() *Service {
	return app.srv
}
