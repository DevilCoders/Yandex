package main

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	l "a.yandex-team.ru/library/go/core/log"
)

const (
	configName = "mdb-pgcheck.yaml"
)

//
// We declare all types here and all fields in them exportable
// so that viper module could unmarshal to them
//

// Config is (surprise) for storing configuration
type Config struct {
	DC                       string        `yaml:"my_dc"`
	Timeout                  time.Duration `yaml:"iteration_timeout"`
	OtherDCPrioIncrease      priority      `yaml:"other_dc_prio_increase"`
	NearPrioMagic            int           `yaml:"near_prio_magic"`
	ReplicationLagMultiplier float32       `yaml:"replication_lag_multiplier"`
	HTTPPort                 int           `yaml:"http_port"`
	LogLevel                 l.Level       `yaml:"log_level"`
	Databases                map[string]DBConfig
}

// DBConfig stores config of a single DB
type DBConfig struct {
	LocalConnString  string `yaml:"local_conn_string"`
	AppendConnString string `yaml:"append_conn_string"`
	Quorum           uint   `yaml:"quorum"`
	Hysterisis       uint   `yaml:"hysterisis"`
}

func defaultConfig() Config {
	dbs := make(map[string]DBConfig)

	return Config{
		DC:                       "DC1",
		Timeout:                  time.Second,
		OtherDCPrioIncrease:      10,
		NearPrioMagic:            5,
		ReplicationLagMultiplier: 1.0,
		HTTPPort:                 8081,
		LogLevel:                 l.InfoLevel,
		Databases:                dbs,
	}
}

func init() {
	flags.RegisterConfigPathFlagGlobal()
}

func parseConfig() Config {
	cfg := defaultConfig()
	if err := config.Load(configName, &cfg); err != nil {
		fmt.Printf("Failed to load application config, using defaults: %s", err)
	}

	return cfg
}
