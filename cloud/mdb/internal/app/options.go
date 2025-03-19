package app

import (
	"reflect"

	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

type AppOption = func(*App)

// WithLoggerConstructor sets logger constructor
func WithLoggerConstructor(lc LoggerConstructor) AppOption {
	return func(app *App) {
		app.newLogger = lc
	}
}

// WithInstrumentation enables instrumentation and endpoints
func WithInstrumentation() AppOption {
	return func(app *App) {
		app.enableInstrumentation = true
	}
}

// WithConfig sets default config. Config must be a pointer to struct and implement AppConfig interface.
// Config will be filled with loaded data if WithConfigLoad was provided.
func WithConfig(cfg AppConfig) AppOption {
	v := reflect.ValueOf(cfg)
	if !v.IsValid() || v.Kind() != reflect.Ptr || v.Elem().Kind() != reflect.Struct {
		panic("config must be a pointer to struct")
	}

	return func(app *App) {
		app.cfg = cfg
	}
}

// WithConfigLoad sets default config filename to load.
// This will make application load config from specified file (using config or config-path from command line if provided).
// If WithConfig was provided, its argument will be filled with loaded data.
func WithConfigLoad(filename string) AppOption {
	flags.RegisterConfigPathFlagGlobal()
	flags.RegisterGenConfigFlagGlobal()
	return func(app *App) {
		app.configFilename = filename
	}
}

// WithReadyChecker enables ready check aggregator
func WithReadyChecker() AppOption {
	return func(app *App) {
		app.readyCheckAggregator = &ready.Aggregator{}
	}
}

// WithSentry enables sentry client
func WithSentry() AppOption {
	return func(app *App) {
		app.enableSentry = true
	}
}

// WithTracing enables tracing
func WithTracing() AppOption {
	return func(app *App) {
		app.enableTracing = true
	}
}

// WithMetrics enables metrics registry
// Combine it with WithMetricsSendOnShutdown if push is needed
func WithMetrics() AppOption {
	return func(app *App) {
		app.enableMetrics = true
	}
}

// WithMetricsSendOnShutdown enables sending sensors on exit only.
func WithMetricsSendOnShutdown() AppOption {
	return func(app *App) {
		app.metricStrategy = SendOnShutdown
	}
}

// WithNoFlagParse disables parsing of flags in App's constructor.
func WithNoFlagParse() AppOption {
	return func(app *App) {
		app.disableFlagParsing = true
	}
}

// DefaultServiceOptions returns application options suitable for most services
func DefaultServiceOptions(cfg AppConfig, filename string) []AppOption {
	return []AppOption{
		WithConfig(cfg),
		WithConfigLoad(filename),
		WithInstrumentation(),
		WithReadyChecker(),
		WithSentry(),
		WithTracing(),
		WithLoggerConstructor(DefaultServiceLoggerConstructor()),
	}
}

// DefaultToolOptions returns application options suitable for most tools
func DefaultToolOptions(cfg AppConfig, filename string) []AppOption {
	return []AppOption{
		WithConfig(cfg),
		WithConfigLoad(filename),
		WithLoggerConstructor(DefaultToolLoggerConstructor()),
	}
}

// DefaultCronOptions returns application options suitable for most cron jobs
func DefaultCronOptions(cfg AppConfig, filename string) []AppOption {
	return []AppOption{
		WithConfig(cfg),
		WithConfigLoad(filename),
		WithSentry(),
		WithTracing(),
		WithLoggerConstructor(DefaultServiceLoggerConstructor()),
	}
}
