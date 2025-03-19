package pg

import (
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	// BackendName is backend's name
	BackendName = "postgresql"
)

const (
	configName = "dbpg.yaml"
)

func defaultConfig() pgutil.Config {
	cfg := pgutil.DefaultConfig()
	cfg.Addrs = []string{"localhost"}
	cfg.DB = "deploydb"
	return cfg
}

func init() {
	deploydb.RegisterBackend(BackendName, NewWithConfigLoad)
}

// LoadConfig loads PostgreSQL db configuration
func LoadConfig(logger log.Logger) (pgutil.Config, error) {
	cfg := defaultConfig()
	if err := config.Load(configName, &cfg); err != nil {
		logger.Errorf("failed to load postgresql db config, using defaults: %s", err)
		cfg = defaultConfig()
	}

	if !cfg.Password.FromEnv("DEPLOYDB_PASSWORD") {
		logger.Info("DEPLOYDB_PASSWORD is empty")
	}

	return cfg, nil
}

// NewWithConfigLoad loads configuration for PostgreSQL db and constructs it
func NewWithConfigLoad(logger log.Logger) (deploydb.Backend, error) {
	cfg, err := LoadConfig(logger)
	if err != nil {
		return nil, err
	}

	return New(cfg, logger)
}
