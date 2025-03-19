package tracing

import (
	"io"

	"github.com/opentracing/opentracing-go"
	"github.com/uber/jaeger-client-go"

	jgrcfg "github.com/uber/jaeger-client-go/config"
)

type dummyCloser struct{}

func (dummyCloser) Close() error {
	return nil
}

func NewTracer(config *Config) (opentracing.Tracer, io.Closer, error) {
	if config == nil {
		return &opentracing.NoopTracer{}, dummyCloser{}, nil
	}

	cfg := jgrcfg.Configuration{
		ServiceName: config.ServiceName,

		Reporter: &jgrcfg.ReporterConfig{
			BufferFlushInterval: config.BufferFlushInterval,
			QueueSize:           config.QueueSize,
			LocalAgentHostPort:  config.LocalAgentHostPort,
		},

		Sampler: &jgrcfg.SamplerConfig{ // send every trace to agent without sampling.
			Type:  jaeger.SamplerTypeConst,
			Param: 1,
		},
	}

	opts := []jgrcfg.Option{
		// TODO: Looger implementsion
		// TODO: service version tag.
		// jgrcfg.Logger(jaeger.StdLogger),
		// jgrcfg.Tag("service.version", buildInfo.SVNVersion),
	}

	return cfg.NewTracer(opts...)
}
