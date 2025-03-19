package internal

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb/clickhouse"
)

// Config describes base config
type Config struct {
	App           app.Config                `json:"app" yaml:"app"`
	API           api.Config                `json:"api" yaml:"api"`
	GRPC          grpcutil.ServeConfig      `json:"grpc" yaml:"grpc"`
	MetaDB        pgutil.Config             `json:"metadb" yaml:"metadb"`
	AccessService AccessServiceConfig       `json:"access_service" yaml:"access_service"`
	DataTransfer  DataTransferServiceConfig `json:"data_transfer" yaml:"data_transfer"`
	LogsDB        clickhouse.Config         `json:"logsdb" yaml:"logsdb"`
	Logic         logic.Config              `json:"logic" yaml:"logic"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App:           app.DefaultConfig(),
		API:           api.DefaultConfig(),
		GRPC:          DefaultGRPCConfig(),
		MetaDB:        pgutil.DefaultConfig(),
		AccessService: DefaultAccessServiceConfig(),
		LogsDB:        clickhouse.DefaultConfig(),
		Logic:         logic.DefaultConfig(),
		DataTransfer:  DefaultDataTransferServiceConfig(),
	}

	cfg.App.Logging.File = "/var/log/logs-api/logs-api.log"
	cfg.App.Tracing.ServiceName = "logs-api"

	return cfg
}

type DataTransferServiceConfig struct {
	Addr         string                `json:"addr" yaml:"addr"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultDataTransferServiceConfig() DataTransferServiceConfig {
	return DataTransferServiceConfig{
		ClientConfig: grpcutil.DefaultClientConfig(),
	}
}

type AccessServiceConfig struct {
	Addr         string                `json:"addr" yaml:"addr"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultAccessServiceConfig() AccessServiceConfig {
	return AccessServiceConfig{
		Addr:         "as.cloud.yandex-team.ru:4286",
		ClientConfig: DefaultGRPCClientConfigWithAllCAs(),
	}
}

func DefaultGRPCClientConfigWithAllCAs() grpcutil.ClientConfig {
	clientConfig := grpcutil.DefaultClientConfig()
	clientConfig.Security.TLS.CAFile = "/opt/yandex/allCAs.pem"
	return clientConfig
}

func DefaultGRPCConfig() grpcutil.ServeConfig {
	cfg := grpcutil.DefaultServeConfig()
	cfg.Addr = ":50050"
	return cfg
}
