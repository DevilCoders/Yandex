package swagger

import (
	"context"
	"net/http"

	openapierrors "github.com/go-openapi/errors"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// Config describes base config
type Config struct {
	LogLevel        log.Level            `json:"loglevel" yaml:"loglevel"`
	LogHTTPBody     bool                 `json:"loghttpbody" yaml:"loghttpbody"`
	Instrumentation httputil.ServeConfig `json:"instrumentation" yaml:"instrumentation"`
}

// DefaultConfig returns default base config
func DefaultConfig() Config {
	return Config{
		LogLevel:        log.DebugLevel,
		Instrumentation: httputil.DefaultInstrumentationConfig(),
	}
}

// App is base application object for go-swagger services
type App struct {
	cfg          Config
	l            log.Logger
	shutdownCtx  context.Context
	shutdownFunc func()
	insthttp     *http.Server
	pinger       pinger
}

// New constructs App
func New(cfg Config, l log.Logger) *App {
	ctx, cancel := context.WithCancel(context.Background())

	srv := httputil.InstrumentationServer(cfg.Instrumentation.Addr, l)
	// TODO: move instrumentation serving to separate func
	// panic is temporary and won't survive the move
	if err := httputil.Serve(srv, l); err != nil {
		panic(err)
	}

	return &App{
		cfg:          cfg,
		l:            l,
		shutdownCtx:  ctx,
		shutdownFunc: cancel,
	}
}

// ShutdownCtx accessor
func (app *App) ShutdownCtx() context.Context {
	return app.shutdownCtx
}

// Logger accessor
func (app *App) Logger() log.Logger {
	return app.l
}

// LogCallback implements logging for go-swagger server
func (app *App) LogCallback(str string, args ...interface{}) {
	app.l.Infof(str, args...)
}

// ServeError logs go-swagger serve errors
func (app *App) ServeError(rw http.ResponseWriter, r *http.Request, err error) {
	ctxlog.Error(r.Context(), app.l, "serve error", log.Error(err))
	openapierrors.ServeError(rw, r, err)
}

// Shutdown initiates application shutdown
func (app *App) Shutdown() {
	app.shutdownFunc()

	if err := httputil.Shutdown(app.insthttp, app.cfg.Instrumentation.ShutdownTimeout); err != nil {
		app.l.Error("error while shutting down instrumentation server", log.Error(err))
	}
}

// PingRegisterer returns PingRegisterer
func (app *App) PingRegisterer() PingRegisterer {
	return &app.pinger
}

// Ping runs all checks. If anything fails, it will return error.
func (app *App) Ping(ctx context.Context) error {
	return app.pinger.Ping(ctx)
}
