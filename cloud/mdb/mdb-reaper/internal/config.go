package internal

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
)

type ResourceManagerConfig struct {
	Target       string                `json:"target" yaml:"target"`
	ClientConfig grpcutil.ClientConfig `json:"client_config" yaml:"client_config"`
}

func DefaultGRPCClientConfigWithAllCAs() grpcutil.ClientConfig {
	clientConfig := grpcutil.DefaultClientConfig()
	clientConfig.Security.TLS.CAFile = "/opt/yandex/allCAs.pem"
	return clientConfig
}

func DefaultResourceManagerConfig() ResourceManagerConfig {
	return ResourceManagerConfig{
		ClientConfig: DefaultGRPCClientConfigWithAllCAs(),
	}
}

type DoubleCloudConfig struct {
	Target       string                `json:"target" yaml:"target"`
	ClientConfig grpcutil.ClientConfig `json:"client_config" yaml:"client_config"`
}

func DefaultDoubleCloudConfig() DoubleCloudConfig {
	return DoubleCloudConfig{
		ClientConfig: DefaultGRPCClientConfigWithAllCAs(),
	}
}

type Config struct {
	App             app.Config            `json:"app" yaml:"app"`
	MetaDB          pgutil.Config         `json:"metadb" yaml:"metadb"`
	ResourceManager ResourceManagerConfig `json:"resource_manager" yaml:"resource_manager"`
	DoubleCloud     DoubleCloudConfig     `json:"double_cloud" yaml:"double_cloud"`
	DryRun          bool                  `json:"dry_run" yaml:"dry_run"`
	StopAllUnused   bool                  `json:"stop_all_unused" yaml:"stop_all_unused"`
}

func (c Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	metaDB := pgutil.DefaultConfig()
	metaDB.Addrs = []string{"localhost"}
	metaDB.DB = "dbaas_metadb"
	metaDB.User = "mdb_reaper"
	return Config{
		App:             app.DefaultConfig(),
		MetaDB:          metaDB,
		ResourceManager: DefaultResourceManagerConfig(),
		DoubleCloud:     DefaultDoubleCloudConfig(),
	}
}
