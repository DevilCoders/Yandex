package pg

import (
	"os"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	// Name is backend's name
	Name = "postgresql"
)

const (
	configName = "mdbhsspg.yaml"
	envAddrs   = "MDBH_SS_PG_ADDRS"
)

func defaultConfig() pgutil.Config {
	config := pgutil.DefaultConfig()
	config.Addrs = []string{"localhost"}
	config.DB = "dbaas_metadb"
	config.User = "postgres"
	return config
}

func init() {
	secretsstore.RegisterBackend(Name, NewWithConfigLoad)
}

// LoadConfig loads PostgreSQL datastore configuration
func LoadConfig(logger log.Logger) (pgutil.Config, error) {
	cfg := defaultConfig()
	if err := config.Load(configName, &cfg); err != nil {
		logger.Errorf("failed to load postgresql secretsstore config, using defaults: %s", err)
		cfg = defaultConfig()
	}

	if addrs, ok := os.LookupEnv(envAddrs); ok {
		cfg.Addrs = strings.Split(addrs, ",")
	}

	if !cfg.Password.FromEnv("METADB_PASSWORD") {
		logger.Info("METADB_PASSWORD is empty")
	}

	return cfg, nil
}

// NewWithConfigLoad loads configuration for PostgreSQL datastore and constructs it
func NewWithConfigLoad(logger log.Logger) (secretsstore.Backend, error) {
	cfg, err := LoadConfig(logger)
	if err != nil {
		return nil, err
	}

	return New(cfg, logger)
}
