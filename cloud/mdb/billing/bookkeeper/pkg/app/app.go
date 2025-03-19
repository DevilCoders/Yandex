package app

import (
	"context"
	"fmt"
	"os"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/bookkeeper"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	bdbpg "a.yandex-team.ru/cloud/mdb/billing/internal/billingdb/pg"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/billing/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	AppName = "bookkeeper"
)

type Config struct {
	App         app.Config            `json:"app" yaml:"app"`
	Bookkeeper  bookkeeper.Config     `json:"bookkeeper" yaml:"bookkeeper"`
	Metadb      pgutil.Config         `json:"metadb" yaml:"metadb"`
	Billingdb   pgutil.Config         `json:"billingdb" yaml:"billingdb"`
	InitTimeout encodingutil.Duration `json:"init_timeout" yaml:"init_timeout"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	return Config{
		App: app.DefaultConfig(),
		Billingdb: pgutil.Config{
			User: "billing_bookkeeper",
			DB:   bdbpg.DBName,
		},
		Metadb: pgutil.Config{
			User: "billing_bookkeeper",
			DB:   mdbpg.DBName,
		},
		Bookkeeper:  bookkeeper.DefaultConfig(),
		InitTimeout: encodingutil.Duration{Duration: time.Second * 10},
	}
}

type BookkeeperApp struct {
	*app.App
	MDB         metadb.MetaDB
	BDB         billingdb.BillingDB
	Keeper      *bookkeeper.Bookkeeper
	InitTimeout time.Duration
	dryrun      bool
}

func newCommonComponentsFromConfig(app *BookkeeperApp, conf Config) error {
	// Initialize metadb
	if !conf.Metadb.Password.FromEnv("METADB_PASSWORD") {
		app.L().Info("METADB_PASSWORD is empty")
	}

	mdb, err := mdbpg.New(conf.Metadb, log.With(app.L(), log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return xerrors.Errorf("prepare MetaDB endpoint: %w", err)
	}
	app.MDB = mdb

	// Initialize billingdb
	if !conf.Billingdb.Password.FromEnv("BILLINGDB_PASSWORD") {
		app.L().Info("BILLINGDB_PASSWORD is empty")
	}

	bdb, err := bdbpg.New(conf.Billingdb, log.With(app.L(), log.String("cluster", bdbpg.DBName)))
	if err != nil {
		return xerrors.Errorf("prepare BillingDB endpoint: %w", err)
	}
	app.BDB = bdb
	app.InitTimeout = conf.InitTimeout.Duration
	return nil
}

func RunBilling(ctx context.Context, app *BookkeeperApp) int {
	app.L().Infof("Bookkeeper started to bill")
	defer app.App.Shutdown()

	if err := ready.Wait(ctx, app.Keeper, &ready.DefaultErrorTester{Name: "databases", L: app.L()}, app.InitTimeout); err != nil {
		fmt.Fprintf(os.Stderr, "bookkeeper failed to start app: %s\n", err)
		return 1
	}

	if err := app.Keeper.Bill(ctx); err != nil {
		app.L().Error("Failed to bill", log.Error(err))
		return 11
	}

	app.L().Infof("Bookkeeper has completed billing successfully")
	return 0
}
