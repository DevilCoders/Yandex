package jaeger

import (
	"github.com/opentracing/opentracing-go"
	"github.com/uber/jaeger-client-go"
	jaegercfg "github.com/uber/jaeger-client-go/config"
	"github.com/uber/jaeger-lib/metrics/prometheus"

	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	ServiceName    string        `json:"service_name" yaml:"service_name"`
	Disabled       bool          `json:"disabled" yaml:"disabled"`
	LocalAgentAddr string        `json:"local_agent_addr" yaml:"local_agent_addr"`
	QueueSize      int           `json:"queue_size" yaml:"queue_size"`
	Sampler        SamplerConfig `json:"sampler" yaml:"sampler"`
}

func DefaultConfig() Config {
	return Config{
		LocalAgentAddr: "localhost:6831",
		QueueSize:      10000,
		Sampler:        DefaultSamplerConfig(),
	}
}

type SamplerConfig struct {
	Type  string  `json:"type" yaml:"type"`
	Param float64 `json:"param" yaml:"param"`
}

func DefaultSamplerConfig() SamplerConfig {
	return SamplerConfig{
		Type:  jaeger.SamplerTypeConst,
		Param: 1,
	}
}

func New(cfg Config, l log.Logger) (*tracing.Tracer, error) {
	jcfg, err := jaegercfg.FromEnv()
	if err != nil {
		l.Warn("could not initialize jaeger tracer", log.Error(err))
		return nil, err
	}

	if jcfg.Sampler.Type == "" {
		jcfg.Sampler.Type = cfg.Sampler.Type
	}
	// We always use sampler param from config since we don't want to bother with float comparison
	jcfg.Sampler.Param = cfg.Sampler.Param

	if jcfg.Reporter.LocalAgentHostPort == "" {
		jcfg.Reporter.LocalAgentHostPort = cfg.LocalAgentAddr
	}
	if jcfg.Reporter.QueueSize == 0 {
		jcfg.Reporter.QueueSize = cfg.QueueSize
	}
	// TODO: headers
	// TODO: throttler

	jMetricsFactory := prometheus.New()

	opts := []jaegercfg.Option{
		jaegercfg.Logger(newLogger(l)),
		jaegercfg.Metrics(jMetricsFactory),
		jaegercfg.Tag("service.version", buildinfo.Info.SVNRevision),
	}

	// Initialize tracer with a logger and a metrics factory
	l.Debugf("initializing global tracer with name %q", cfg.ServiceName)
	closer, err := jcfg.InitGlobalTracer(
		cfg.ServiceName,
		opts...,
	)
	if err != nil {
		l.Warn("could not initialize jaeger tracer", log.Error(err))
		return nil, err
	}

	l.Debug("jaeger tracer configured", log.String("local_agent_addr", cfg.LocalAgentAddr))
	return &tracing.Tracer{Tracer: opentracing.GlobalTracer(), Closer: closer}, nil
}
