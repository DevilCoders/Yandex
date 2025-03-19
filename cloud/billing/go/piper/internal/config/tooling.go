package config

import (
	"time"

	"github.com/uber/jaeger-client-go"
	jgrcfg "github.com/uber/jaeger-client-go/config"
	ozap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracing"
	"a.yandex-team.ru/cloud/billing/go/pkg/ualogs"
	"a.yandex-team.ru/cloud/billing/go/pkg/zapjournald"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type toolingContainer struct {
	loggingOnce initSync
	metricsOnce initSync
	tracingOnce initSync
}

func (c *Container) initializedTooling() {
	c.initializedLogging()
	c.initializedMetrics()
	c.initializedTracing()
}

func (c *Container) initializedLogging() {
	_ = c.loggingOnce.Do(c.configureLogging)
}

func (c *Container) initializedMetrics() {
	_ = c.metricsOnce.Do(c.configureMetrics)
}

func (c *Container) initializedTracing() {
	_ = c.tracingOnce.Do(c.configureTracing)
}

func (c *Container) configureLogging() error {
	if c.initFailed() {
		return c.initError
	}
	config, err := c.GetLoggingConfig()
	if err != nil {
		return err
	}

	err = ualogs.RegisterSink()
	if err != nil {
		return c.failInitF("register logbroker zap sink: %w", err)
	}

	zapCfg := zap.JSONConfig(config.Level)

	// We use logger from strongly wrapped tooling. Stack traces are not usefull.
	zapCfg.DisableCaller = true
	zapCfg.DisableStacktrace = true

	if len(config.Paths) > 0 {
		zapCfg.OutputPaths = config.Paths
	}

	logger, err := zap.New(zapCfg)
	if err != nil {
		return c.failInitF("zap logger create: %w", err)
	}
	if config.EnableJournald.Bool() {
		jdCore := zapjournald.NewCore(zap.ZapifyLevel(config.Level))
		logger.L = logger.L.WithOptions(
			ozap.WrapCore(func(c zapcore.Core) zapcore.Core {
				return zapcore.NewTee(c, jdCore)
			}),
		)
	}

	c.statesMgr.Add("logger", logger)

	logging.SetLogger(logger)
	c.logger = logger
	return nil
}

func (c *Container) configureMetrics() error {
	cfgVersion, _ := c.GetConfigVersion()
	if c.initFailed() {
		return c.initError
	}
	srvVersion := "custom"
	if cfgVersion != "" { // TODO: think more carefully about code version detection
		srvVersion = cfgVersion
	}

	metrics.SetConfigVersion(cfgVersion)
	metrics.SetServiceVersion(srvVersion)
	return nil
}

func (c *Container) configureTracing() error {
	c.initializedLogging()

	if c.initFailed() {
		return c.initError
	}

	trCfg, err := c.GetTracingConfig()
	if err != nil {
		return err
	}

	reporter := jgrcfg.ReporterConfig{}
	if trCfg.Enabled.Bool() {
		reporter.BufferFlushInterval = time.Second
		reporter.QueueSize = trCfg.QueueSize
		reporter.LocalAgentHostPort = trCfg.LocalAgentHostPort
	}

	cfg := jgrcfg.Configuration{
		ServiceName: "yc-billing-piper",
		Disabled:    !trCfg.Enabled.Bool(),
		Reporter:    &reporter,
		Sampler: &jgrcfg.SamplerConfig{
			Type:  jaeger.SamplerTypeConst,
			Param: 1,
		},
	}

	opts := []jgrcfg.Option{
		jgrcfg.Logger(newJaegerLogger(logging.Logger())),
		// jgrcfg.Tag("service.version", buildinfo.Info.SVNRevision),
	}

	tracer, closer, err := cfg.NewTracer(opts...)
	if err != nil {
		return c.failInitF("tracing init: %w", err)
	}
	c.statesMgr.Add("_trace_closer", closer)
	tracing.SetTracer(tracer, closer)
	return nil
}
