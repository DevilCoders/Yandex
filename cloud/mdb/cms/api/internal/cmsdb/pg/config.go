package pg

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	configName       = "dbpg.yaml"
	metadbConfigName = "metadb.yaml"
)

func DefaultConfig() pgutil.Config {
	config := pgutil.DefaultConfig()
	config.Addrs = []string{"localhost"}
	config.DB = "cmsdb"
	config.User = "cms"
	return config
}

// LoadConfig loads PostgreSQL db configuration
func LoadConfig(logger log.Logger, filename string) (pgutil.Config, error) {
	cfg := DefaultConfig()
	if err := config.Load(filename, &cfg); err != nil {
		logger.Errorf("failed to load postgresql db config '%s': %s", filename, err)
		return cfg, err
	}

	return cfg, nil
}

// NewCMSDBWithConfigLoad loads configuration for PostgreSQL db and constructs it
func NewCMSDBWithConfigLoad(logger log.Logger) (cmsdb.Client, error) {
	cfg, err := LoadConfig(logger, configName)
	if err != nil {
		return &Backend{}, err
	}
	return New(cfg, log.With(logger, log.String("cluster", cfg.DB)))
}

// NewMetaDBWithConfigLoad loads configuration for PostgreSQL db and constructs it
func NewMetaDBWithConfigLoad(logger log.Logger) (metadb.MetaDB, error) {
	cfg, err := LoadConfig(logger, metadbConfigName)
	if err != nil {
		return nil, err
	}
	return metadbpg.New(cfg, log.With(logger, log.String("cluster", cfg.DB)))
}
