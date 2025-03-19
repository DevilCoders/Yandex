package config

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
)

const (
	VpcPasswdEnvName = "VPCDB_PASSWORD"
)

type Config struct {
	App          app.Config           `json:"app" yaml:"app"`
	Retry        retry.Config         `json:"retry" yaml:"retry"`
	GRPC         grpcutil.ServeConfig `json:"grpc" yaml:"grpc"`
	SLBCloseFile string               `json:"slb_close_file" yaml:"slb_close_file"`
	VPCDB        pgutil.Config        `json:"vpcdb" yaml:"vpcdb"`
	AWS          Aws                  `json:"aws" yaml:"aws"`
	Auth         AuthConfig           `json:"auth" yaml:"auth"`
	AWSSA        aws.SAConfig         `json:"aws_sa" yaml:"aws_sa"`
	Regions      Regions              `json:"regions" yaml:"regions"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

type Aws struct {
	HTTP             httputil.ClientConfig `json:"http" yaml:"http"`
	DataplaneRoleArn string                `json:"dataplane_role_arn" yaml:"dataplane_role_arn"`
}

type AuthConfig struct {
	Addr   string                `json:"addr" yaml:"addr"`
	Config grpcutil.ClientConfig `json:"config" yaml:"config"`
}

type Regions struct {
	AWS []string `json:"aws" yaml:"aws"`
}

func DefaultAwsConfig() Aws {
	return Aws{
		HTTP: httputil.ClientConfig{
			Transport: httputil.DefaultTransportConfig(),
		},
	}
}

func DefaultConfig() *Config {
	cfg := &Config{
		App:          app.DefaultConfig(),
		Retry:        retry.DefaultConfig(),
		GRPC:         grpcutil.DefaultServeConfig(),
		SLBCloseFile: "/tmp/.mdb-vpc-api-close",
		VPCDB:        pgutil.DefaultConfig(),
		AWSSA:        aws.DefaultSAConfig(),
		AWS:          DefaultAwsConfig(),
	}

	cfg.VPCDB.Password.FromEnv(VpcPasswdEnvName)

	return cfg
}
