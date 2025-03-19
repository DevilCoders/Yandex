package main

import (
	"a.yandex-team.ru/cloud/ps/gore/internal/app/api"
	"a.yandex-team.ru/cloud/ps/gore/internal/app/config"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/abcclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/calendarclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/csv"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/golemclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/idmclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/jugglerclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/mongoclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/stclient"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

func main() {
	// Init system-wide logger
	l, err := zap.NewQloudLogger(log.DebugLevel)
	if err != nil {
		panic(err)
	}

	// Init bootstrap-only logger
	li := l.With(log.String("module", "init"))

	// TVM and Blackbox clients
	tvm, err := tvmtool.NewQloudClient()
	if err != nil {
		li.Errorf("Failed to create TVM client")
		panic(err)
	}

	bb, err := httpbb.NewIntranet(
		httpbb.WithLogger(l),
		httpbb.WithTVM(tvm),
	)
	if err != nil {
		li.Errorf("Failed to create Blackbox client")
		panic(err)
	}

	// Merge configs
	c := &config.Config{
		Logger:   l,
		Startrek: stclient.DefaultConfig(),
		Calendar: calendarclient.DefaultConfig(),
		Golem:    golemclient.DefaultConfig(),
		IDM:      idmclient.DefaultConfig(),
		Juggler:  jugglerclient.DefaultConfig(),
		CSV:      csv.DefaultConfig(),
		Mongo:    mongoclient.DefaultConfig(),
		ABC:      abcclient.DefaultConfig(),
	}
	err = c.LoadConfigs()
	if err != nil {
		li.Errorf("Failed to read configuration")
		panic(err)
	}

	// Create package-specific loggers
	c.Calendar.Log = l.With(log.String("module", "calendar"))
	c.Juggler.Log = l.With(log.String("module", "juggler"))
	c.IDM.Log = l.With(log.String("module", "idm"))
	c.Golem.Log = l.With(log.String("module", "golem"))
	c.Startrek.Log = l.With(log.String("module", "startrek"))
	c.CSV.Log = l.With(log.String("module", "csv-parser"))
	c.Mongo.Log = l.With(log.String("module", "mongo"))
	c.ABC.Log = l.With(log.String("module", "abc"))

	// Establish database connection
	err = c.Mongo.InitDBConnection()
	if err != nil {
		li.Errorf("Failed to connect to specified database")
		panic(err)
	}

	// Update packages, pelying on external datastore
	c.Startrek.Storage = c.Mongo
	c.Juggler.Storage = c.Mongo

	srv := &api.Server{
		BaseLogger: l,
		Config:     c,
		BBClient:   bb,
	}
	li.Infof("Configuration loaded successfuly")
	api.Mux(srv)
}
