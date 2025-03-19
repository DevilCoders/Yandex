package app

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/logbroker"
	mdbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/producer"
)

// Config describes service config
type Config struct {
	App       app.Config       `json:"app" yaml:"app"`
	Producer  producer.Config  `json:"producer" yaml:"producer"`
	MetaDB    pgutil.Config    `json:"metadb" yaml:"metadb"`
	LogBroker logbroker.Config `json:"logbroker" yaml:"logbroker"`
}

// DefaultConfig returns default base config
func DefaultConfig() Config {
	lbCfg := logbroker.DefaultConfig()
	lbCfg.SourceID = appName
	lbCfg.TVM.ClientAlias = appName
	return Config{
		App:       app.DefaultConfig(),
		Producer:  producer.DefaultConfig(),
		LogBroker: lbCfg,
		MetaDB: pgutil.Config{
			User: "mdb_search_producer",
			DB:   mdbpg.DBName,
		},
	}
}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

const (
	ConfigName = "mdb-search-producer.yaml"
	appName    = "mdb-search-producer"
)
