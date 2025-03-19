package app

import (
	"fmt"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/deprecated/app"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	lb_writer "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/logbroker"
	mdbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/producer"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// Config describes service config
type Config struct {
	Config    app.Config       `json:"config" yaml:"config"`
	Producer  producer.Config  `json:"producer" yaml:"producer"`
	MetaDB    pgutil.Config    `json:"metadb" yaml:"metadb"`
	LogBroker lb_writer.Config `json:"logbroker" yaml:"logbroker"`
}

// DefaultConfig returns default base config
func DefaultConfig() Config {
	lbCfg := lb_writer.DefaultConfig()
	lbCfg.SourceID = appName
	lbCfg.TVM.ClientAlias = appName
	return Config{
		Config:    app.DefaultConfig(),
		Producer:  producer.DefaultConfig(),
		LogBroker: lbCfg,
		MetaDB: pgutil.Config{
			User: "mdb_event_producer",
			DB:   mdbpg.DBName,
		},
	}
}

const (
	configName      = "mdb-event-producer.yaml"
	flagKeyLogLevel = "mdb-loglevel"
	appName         = "mdb-event-producer"
)

var appFlags *pflag.FlagSet

func init() {
	defcfg := DefaultConfig()
	appFlags = pflag.NewFlagSet("App", pflag.ExitOnError)
	appFlags.String(flagKeyLogLevel, defcfg.Config.LogLevel.String(), "Set log level")
	pflag.CommandLine.AddFlagSet(appFlags)
	flags.RegisterConfigPathFlagGlobal()
}

func LoadConfig() Config {
	// Default logger so we can log something even when we have no configuration loaded
	lg, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		fmt.Printf("failed to initialize logger: %s\n", err)
		os.Exit(1)
	}

	cfg := DefaultConfig()
	if err = config.Load(configName, &cfg); err != nil {
		lg.Errorf("failed to load application config: %s", err)
		os.Exit(1)
	}

	if appFlags.Changed(flagKeyLogLevel) {
		var ll string
		if ll, err = appFlags.GetString(flagKeyLogLevel); err == nil {
			cfg.Config.LogLevel, err = log.ParseLevel(ll)
			if err != nil {
				lg.Fatalf("failed to parse loglevel: %s", err)
			}
		}
	}

	return cfg
}
