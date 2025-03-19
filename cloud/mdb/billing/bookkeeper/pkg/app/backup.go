package app

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/bookkeeper"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer/simplebackup"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BackupBookkeeperAppConfig struct {
	Config `yaml:",inline"`
}

var _ app.AppConfig = &BackupBookkeeperAppConfig{}

func DefaultBackupBookkeeperAppConfig() BackupBookkeeperAppConfig {
	return BackupBookkeeperAppConfig{
		Config: DefaultConfig(),
	}
}

type BackupBookkeeperApp struct {
	BookkeeperApp
	Config BackupBookkeeperAppConfig
}

func NewBackupBookkeeperAppFromConfig(ctx context.Context) (*BackupBookkeeperApp, error) {
	conf := DefaultBackupBookkeeperAppConfig()
	opts := app.DefaultToolOptions(&conf, fmt.Sprintf("%s.yaml", AppName))
	opts = append(opts, app.WithMetrics())
	opts = append(opts, app.WithMetricsSendOnShutdown())
	baseApp, err := app.New(opts...)
	if err != nil {
		return nil, xerrors.Errorf("make base app, %w", err)
	}

	a := BackupBookkeeperApp{
		BookkeeperApp: BookkeeperApp{
			App: baseApp,
		},
		Config: conf,
	}

	if err := newCommonComponentsFromConfig(&a.BookkeeperApp, conf.Config); err != nil {
		return nil, err
	}

	keeper, err := NewBackupBookkeeperFromExtDeps(a.MDB, a.BDB, a.L(), a.Config.Bookkeeper)
	if err != nil {
		return nil, err
	}

	a.Keeper = keeper
	return &a, err
}

func NewBackupBookkeeperFromExtDeps(mdb metadb.MetaDB, bdb billingdb.BillingDB, logger log.Logger, cfg bookkeeper.Config) (*bookkeeper.Bookkeeper, error) {
	calc := map[metadb.ClusterType]invoicer.Invoicer{
		metadb.PostgresqlCluster: simplebackup.NewSimpleBackupInvoicer(mdb),
	}

	hostname, err := os.Hostname()
	if err != nil {
		return nil, err
	}

	exp := exporter.NewBillingExporter(
		generator.NewUUIDGenerator(),
		hostname,
	)

	mb, err := exporter.NewYCMetricsBatcher(generator.NewUUIDGenerator())
	if err != nil {
		return nil, err
	}
	return bookkeeper.New(billingdb.BillTypeBackup, calc, exp, mdb, bdb, mb, logger, cfg), nil
}
