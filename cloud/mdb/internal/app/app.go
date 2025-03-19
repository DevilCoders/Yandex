package app

import (
	"context"
	"net/http"
	"os"
	"sync"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sentry/raven"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/jaeger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/prometheus"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/pusher/httppusher"
)

type MetricPusherStrategy int

const (
	WithoutPush MetricPusherStrategy = iota
	SendOnShutdown
)

type App struct {
	l                     log.Logger
	cfg                   AppConfig
	configFilename        string
	enableInstrumentation bool
	newLogger             LoggerConstructor
	enableSentry          bool
	enableTracing         bool
	disableFlagParsing    bool

	shutdownCtx         context.Context
	shutdownFunc        context.CancelFunc
	shutdownCallbacksMu sync.Mutex
	shutdownCallbacks   []func()

	insthttp *http.Server

	readyCheckAggregator *ready.Aggregator
	// We do not expose tracer. Right now we support only one tracer and it is accessible
	// via opentracing.GlobalTracer()
	tracer *tracing.Tracer

	// All configs related to metrics. Handles both Metrics and Prometheus
	enableMetrics  bool
	metricStrategy MetricPusherStrategy
	metricRegistry metrics.Registry
	solomonPusher  *httppusher.Pusher

	saCredentialsService iam.CredentialsService
}

// New creates new application instance.
// One of these happens:
// - returns new application
// - returns an error
// - performs os.Exit(0) if one-time action was requested (like config generation)
func New(opts ...AppOption) (*App, error) {
	ctx, cancel := context.WithCancel(context.Background())
	// Create default app
	a := &App{
		cfg:          defaultConfigPtr(),
		shutdownCtx:  ctx,
		shutdownFunc: cancel,
		newLogger:    DefaultLoggerConstructor(),
	}

	// Apply options
	for _, opt := range opts {
		opt(a)
	}

	// Parse command line arguments
	a.parseFlags()
	// Generate or load config if filename was provided
	if a.configFilename != "" {
		ok, err := config.GenerateConfigOnFlag(a.configFilename, a.cfg)
		if err != nil {
			return nil, err
		}
		if ok {
			os.Exit(0)
		}

		if err := config.Load(a.configFilename, a.cfg); err != nil {
			return nil, xerrors.Errorf("failed to load application config: %w", err)
		}
	}

	// Apply log level from command line
	level, ok, err := flags.FlagLogLevel()
	if err != nil {
		return nil, err
	}
	if ok {
		a.cfg.AppConfig().Logging.Level = level
	}

	// Create logger
	if a.l, err = a.newLogger(a.cfg.AppConfig().Logging); err != nil {
		return nil, xerrors.Errorf("failed to initialize logger: %s", err)
	}

	a.l.Debug("using application config", log.Any("config", a.cfg))

	// This must be initialized first because it sets global tracer which
	// can be used during other initializations
	if a.enableTracing {
		a.tracer, err = jaeger.New(a.cfg.AppConfig().Tracing, a.l)
		if err != nil {
			a.l.Warn("running without tracing", log.Error(err))
		}
	}

	if a.enableInstrumentation {
		a.insthttp = httputil.InstrumentationServer(a.cfg.AppConfig().Instrumentation.Addr, a.l)
		if err := httputil.Serve(a.insthttp, a.l); err != nil {
			return nil, err
		}
	}

	if a.enableSentry {
		sentryCfg := a.cfg.AppConfig().Sentry
		if sentryCfg.DSN.FromEnv("SENTRY_DSN") {
			a.l.Info("gоt sentry dsn from env")
		}
		if err = raven.Init(sentryCfg); err != nil {
			a.l.Warn("running without sentry", log.Error(err))
		}
	}

	if !a.cfg.AppConfig().ServiceAccount.Empty() {
		appCfg := a.cfg.AppConfig()
		envCfg := appCfg.Environment
		client, err := grpc.NewTokenServiceClient(
			ctx,
			envCfg.Services.Iam.V1.TokenService.Endpoint,
			appCfg.AppName,
			grpcutil.DefaultClientConfig(),
			&grpcutil.PerRPCCredentialsStatic{},
			a.l,
		)
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize token service client %w", err)
		}
		a.saCredentialsService = client.ServiceAccountCredentials(iam.ServiceAccount{
			ID:    appCfg.ServiceAccount.ID,
			KeyID: appCfg.ServiceAccount.KeyID.Unmask(),
			Token: []byte(appCfg.ServiceAccount.PrivateKey.Unmask()),
		})

	}

	if err := a.initializeMetrics(); err != nil {
		return nil, err
	}

	return a, nil
}

func (a *App) initializeMetrics() error {
	if a.enableMetrics {
		if a.cfg.AppConfig().Solomon.URL != "" && a.cfg.AppConfig().Prometheus.URL != "" {
			return xerrors.Errorf("both Solomon and Prometheus configs are filled")
		}

		if a.cfg.AppConfig().Solomon.URL != "" {
			registryOpts := solomon.NewRegistryOpts().
				SetUseNameTag(a.cfg.AppConfig().Solomon.UseNameTag)
			a.metricRegistry = solomon.NewRegistry(registryOpts)
			if a.metricStrategy != WithoutPush {
				solomonCfg := a.cfg.AppConfig().Solomon
				pusherOpts := []httppusher.PusherOpt{
					httppusher.SetProject(solomonCfg.Project),
					httppusher.SetService(solomonCfg.Service),
					httppusher.SetCluster(solomonCfg.Cluster),
					httppusher.WithHTTPHost(solomonCfg.URL),
					httppusher.WithLogger(a.l.Structured()),
				}

				if solomonCfg.OAuthToken != "" {
					pusherOpts = append(pusherOpts, httppusher.WithOAuthToken(solomonCfg.OAuthToken))
				} else if a.ServiceAccountCredentials() != nil {
					pusherOpts = append(pusherOpts, httppusher.WithIAMTokenProvider(func(ctx context.Context) (string, error) {
						token, err := a.saCredentialsService.Token(ctx)
						if err != nil {
							a.l.Errorf("failed to get iam token %v", err)
							return "", err
						}
						return token, nil
					}))
				} else {
					return xerrors.Errorf("neither Service Account nor OAuth tokens are not filled")
				}

				var err error
				if a.solomonPusher, err = httppusher.NewPusher(pusherOpts...); err != nil {
					return err
				}
			}
		} else if a.cfg.AppConfig().Prometheus.URL != "" {
			prometheusCfg := a.cfg.AppConfig().Prometheus
			registryOpts := prometheus.NewRegistryOpts().
				SetPrefix(prometheusCfg.Namespace)
			a.metricRegistry = prometheus.NewRegistry(registryOpts)
			// TODO prometheus push to pushgateway
		} else {
			return xerrors.Errorf("neither Solomon nor Prometheus configs are not filled")
		}
	}

	return nil
}

func (a *App) WaitForStop() {
	signals.WaitForStop()
	a.Shutdown()
}

func (a *App) Shutdown() {
	a.shutdownFunc()

	if a.enableInstrumentation {
		if err := httputil.Shutdown(a.insthttp, a.cfg.AppConfig().Instrumentation.ShutdownTimeout); err != nil {
			a.l.Error("error while shutting down instrumentation server", log.Error(err))
		}
	}

	if a.metricStrategy == SendOnShutdown {
		a.pushMetrics()
	}

	// Tracer might not exist (in tests for example)
	if a.enableTracing && a.tracer != nil {
		if err := a.tracer.Close(); err != nil {
			a.L().Error("error while closing tracer", log.Error(err))
		}
	}

	a.shutdownCallbacksMu.Lock()
	defer a.shutdownCallbacksMu.Unlock()
	for _, callback := range a.shutdownCallbacks {
		callback()
	}
}

func (a *App) pushMetrics() {
	if a.solomonPusher != nil {
		sensors, err := a.metricRegistry.(*solomon.Registry).Gather()
		if err != nil {
			a.L().Error("error while getting sensors from solomon registry", log.Error(err))
		} else if err = a.solomonPusher.Push(context.Background(), sensors); err != nil {
			a.L().Error("error while sending sensors to solomon", log.Error(err))
		}
	}
	// TODO push Prometheus metrics to pushgateway
}

func (a *App) parseFlags() {
	if a.disableFlagParsing {
		return
	}
	if pflag.CommandLine.Parsed() {
		return
	}
	pflag.Parse()
}

func (a *App) ShutdownContext() context.Context {
	return a.shutdownCtx
}

func (a *App) RegisterShutdownCallback(callback func()) {
	a.shutdownCallbacksMu.Lock()
	defer a.shutdownCallbacksMu.Unlock()
	a.shutdownCallbacks = append(a.shutdownCallbacks, callback)
}

func (a *App) L() log.Logger {
	return a.l
}

// ReadyCheckAggregator returns ready check aggregator
func (a *App) ReadyCheckAggregator() *ready.Aggregator {
	return a.readyCheckAggregator
}

// Metrics returns solomon registry if it was enabled and initialized
func (a *App) Metrics() metrics.Registry {
	return a.metricRegistry
}

// ServiceAccountCredentials returns iam.CredentialsService if it was enabled and initialized
func (a *App) ServiceAccountCredentials() iam.CredentialsService {
	return a.saCredentialsService
}
