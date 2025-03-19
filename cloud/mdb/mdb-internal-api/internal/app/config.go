package app

import (
	"time"

	httpmarketplace "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace/http"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	healthswagger "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	chlogsdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse"
	pgmdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/pg"
)

type MetaDBConfig struct {
	DB pgutil.Config `json:"db" yaml:"db"`
}

func DefaultMetaDBConfig() MetaDBConfig {
	return MetaDBConfig{
		DB: pgmdb.DefaultConfig(),
	}
}

type LogsDBConfig struct {
	Disabled        bool `json:"disabled" yaml:"disabled"`
	chlogsdb.Config `json:"config" yaml:"config"`
}

func DefaultLogsDBConfig() LogsDBConfig {
	return LogsDBConfig{
		Config: chlogsdb.DefaultConfig(),
	}
}

type SLBCloseFileConfig struct {
	FilePath string `json:"file_path" yaml:"file_path"`
}

type ReadOnlyFileConfig struct {
	FilePath string `json:"file_path" yaml:"file_path"`
}

func DefaultGRPCConfig() grpcutil.ServeConfig {
	cfg := grpcutil.DefaultServeConfig()
	cfg.Addr = ":50050"
	return cfg
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

type LicenseServiceConfig struct {
	Addr       string                       `json:"addr" yaml:"addr"`
	HTTPConfig httpmarketplace.ClientConfig `json:"http_config" yaml:"http_config"`
}

func DefaultLicenseServiceConfig() LicenseServiceConfig {
	return LicenseServiceConfig{
		HTTPConfig: httpmarketplace.DefaultClientConfig(),
	}
}

func DefaultHealthConfig() healthswagger.Config {
	return healthswagger.Config{
		TLS: httputil.TLSConfig{
			CAFile: "/opt/yandex/allCAs.pem",
		},
	}
}

type TokenServiceConfig struct {
	Addr         string                `json:"addr" yaml:"addr"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultTokenServiceConfig() TokenServiceConfig {
	return TokenServiceConfig{
		ClientConfig: DefaultGRPCClientConfigWithAllCAs(),
	}
}

type ResourceManagerConfig struct {
	Addr         string                `json:"addr" yaml:"addr"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultResourceManagerConfig() ResourceManagerConfig {
	clientConfig := DefaultGRPCClientConfigWithAllCAs()
	// resource manager responds slowly to some methods: https://st.yandex-team.ru/MDB-18088
	clientConfig.Retries.PerRetryTimeout = 5 * time.Second
	return ResourceManagerConfig{
		ClientConfig: clientConfig,
	}
}

type CryptoConfig struct {
	PeersPublicKey string        `json:"peers_public_key" yaml:"peers_public_key"`
	PrivateKey     secret.String `json:"private_key" yaml:"private_key"`
}

func DefaultCryptoConfig() CryptoConfig {
	return CryptoConfig{}
}

func DefaultGRPCClientConfigWithAllCAs() grpcutil.ClientConfig {
	clientConfig := grpcutil.DefaultClientConfig()
	clientConfig.Security.TLS.CAFile = "/opt/yandex/allCAs.pem"
	return clientConfig
}

type PillarSecretServiceConfig struct {
	Addr         string                `json:"addr" yaml:"addr"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultPillarSecretServiceConfig() PillarSecretServiceConfig {
	return PillarSecretServiceConfig{
		ClientConfig: DefaultGRPCClientConfigWithAllCAs(),
	}
}

type VpcAPIConfig struct {
	Address              string                `json:"addr" yaml:"addr"`
	DefaultIpv4CidrBlock string                `json:"default_ipv4_cidr_block" yaml:"default_ipv4_cidr_block"`
	ClientConfig         grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultVpcAPIConfig() VpcAPIConfig {
	return VpcAPIConfig{
		DefaultIpv4CidrBlock: "172.42.0.0/16",
		ClientConfig:         DefaultGRPCClientConfigWithAllCAs(),
	}
}
