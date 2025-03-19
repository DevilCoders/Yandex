package internal

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/api"
	pgmdb "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/metadb/pg"
)

// Config describes base config
type Config struct {
	App           app.Config           `json:"app" yaml:"app"`
	API           api.Config           `json:"api" yaml:"api"`
	GRPC          grpcutil.ServeConfig `json:"grpc" yaml:"grpc"`
	MetaDB        pgutil.Config        `json:"metadb" yaml:"metadb"`
	AccessService AccessServiceConfig  `json:"access_service" yaml:"access_service"`
	Crypto        CryptoConfig         `json:"crypto" yaml:"crypto"`
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
		MetaDB:        pgmdb.DefaultConfig(),
		AccessService: DefaultAccessServiceConfig(),
		Crypto:        DefaultCryptoConfig(),
	}

	cfg.App.Logging.File = "/var/log/mdb-pillar-secrets/mdb-pillar-secrets.log"
	cfg.App.Tracing.ServiceName = "mdb-pillar-secrets"

	return cfg
}

type CryptoConfig struct {
	PeersPublicKey string        `json:"peers_public_key" yaml:"peers_public_key"`
	PrivateKey     secret.String `json:"private_key" yaml:"private_key"`
}

func DefaultCryptoConfig() CryptoConfig {
	return CryptoConfig{}
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
