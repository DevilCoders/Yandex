package app

import (
	"context"
	"fmt"
	"os"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/bookkeeper"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer/cloudstorage"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	s3http "a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type CloudStorageBookkeeperAppConfig struct {
	Config             `yaml:",inline"`
	S3                 s3http.Config               `json:"s3_config" yaml:"s3_config"`
	S3Report           cloudstorage.S3ReportConfig `json:"s3_report" yaml:"s3_report"`
	BillingThresholdMB int64                       `json:"billing_threshold" yaml:"billing_threshold"`
}

var _ app.AppConfig = &CloudStorageBookkeeperAppConfig{}

func DefaultCloudStorageBookkeeperAppConfig() CloudStorageBookkeeperAppConfig {
	return CloudStorageBookkeeperAppConfig{
		Config:             DefaultConfig(),
		S3:                 s3http.DefaultConfig(),
		BillingThresholdMB: 16,
	}
}

type CloudStorageBookkeeperApp struct {
	BookkeeperApp
	Config CloudStorageBookkeeperAppConfig
}

func NewCloudStorageBookkeeperAppFromConfig(ctx context.Context) (*CloudStorageBookkeeperApp, error) {
	conf := DefaultCloudStorageBookkeeperAppConfig()
	opts := app.DefaultToolOptions(&conf, fmt.Sprintf("%s.yaml", AppName))
	opts = append(opts, app.WithMetrics())
	opts = append(opts, app.WithMetricsSendOnShutdown())
	baseApp, err := app.New(opts...)
	if err != nil {
		return nil, xerrors.Errorf("make base app, %w", err)
	}

	a := CloudStorageBookkeeperApp{
		BookkeeperApp: BookkeeperApp{
			App: baseApp,
		},
		Config: conf,
	}

	if err := newCommonComponentsFromConfig(&a.BookkeeperApp, conf.Config); err != nil {
		return nil, err
	}

	// Initialize S3 client
	if !a.Config.S3.AccessKey.FromEnv("S3_ACCESS_KEY") {
		a.L().Info("S3_ACCESS_KEY is empty")
	}

	if !a.Config.S3.SecretKey.FromEnv("S3_SECRET_KEY") {
		a.L().Info("S3_SECRET_KEY is empty")
	}

	s3, err := s3http.New(a.Config.S3, a.L())
	if err != nil {
		return nil, xerrors.Errorf("prepare s3 client: %w", err)
	}

	report, err := cloudstorage.NewS3Report(ctx, a.Config.S3Report, s3, a.L(), time.Now())
	if err != nil {
		return nil, xerrors.Errorf("load cost and usage report: %s", err.Error())
	}

	calc := map[metadb.ClusterType]invoicer.Invoicer{
		metadb.ClickhouseCluster: cloudstorage.NewAWSUsageReportInvoicer(a.MDB, report, conf.BillingThresholdMB, a.L()),
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

	a.Keeper = bookkeeper.New(billingdb.BillTypeCloudStorage, calc, exp, a.MDB, a.BDB, mb, a.L(), a.Config.Bookkeeper)
	return &a, err
}
