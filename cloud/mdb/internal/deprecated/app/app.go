package app

import (
	"context"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
)

// Config describes base config
type Config struct {
	LogLevel        log.Level            `json:"log_level" yaml:"log_level"`
	LogFile         string               `json:"log_file" yaml:"log_file"`
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

// Shutdown initiates application shutdown
func (app *App) Shutdown() {
	app.shutdownFunc()

	if err := httputil.Shutdown(app.insthttp, app.cfg.Instrumentation.ShutdownTimeout); err != nil {
		app.l.Error("error while shutting down instrumentation server", log.Error(err))
	}
}
