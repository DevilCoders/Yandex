package agentapp

import (
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/call"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/srv"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource/s3"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/defaults"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
)

type Config struct {
	App              app.Config           `json:"app" yaml:"app"`
	GRPC             grpcutil.ServeConfig `json:"grpc" yaml:"grpc"`
	Agent            agent.Config         `json:"agent" yaml:"agent"`
	ExposeErrorDebug bool                 `json:"expose_error_debug" yaml:"expose_error_debug"`
	Retry            retry.Config         `json:"retry" yaml:"retry"`
	S3               s3.Config            `json:"s3" yaml:"s3"`
	Call             call.Config          `json:"call" yaml:"call"`
	Srv              srv.Config           `json:"srv" yaml:"srv"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App:              app.DefaultConfig(),
		GRPC:             grpcutil.DefaultServeConfig(),
		Agent:            agent.DefaultConfig(),
		ExposeErrorDebug: true,
		Retry:            retry.DefaultConfig(),
		S3:               s3.DefaultConfig(),
		Call:             call.DefaultConfig(),
		Srv:              srv.DefaultConfig(),
	}
	cfg.GRPC.Addr = defaults.SocketPath
	cfg.App.Tracing.Disabled = true
	return cfg
}
