package app

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	bdbpg "a.yandex-team.ru/cloud/mdb/billing/internal/billingdb/pg"
	"a.yandex-team.ru/cloud/mdb/billing/sender/internal/sender"
	"a.yandex-team.ru/cloud/mdb/billing/sender/internal/writer/kinesis"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type KinesisAppConfig struct {
	Config  `yaml:",inline"`
	Kinesis kinesis.Config `json:"kinesis" yaml:"kinesis"`
}

var _ app.AppConfig = &KinesisAppConfig{}

func DefaultKinesisAppConfig() KinesisAppConfig {
	return KinesisAppConfig{Config: DefaultConfig()}
}

type KinesisSenderApp struct {
	SenderApp
	Config KinesisAppConfig
}

func NewKinesisMetricsSenderAppFromConfig(ctx context.Context, btype billingdb.BillType) (*KinesisSenderApp, error) {
	conf := DefaultKinesisAppConfig()
	opts := app.DefaultToolOptions(&conf, fmt.Sprintf("%s.yaml", AppName))
	opts = append(opts, app.WithMetrics())
	opts = append(opts, app.WithMetricsSendOnShutdown())

	baseApp, err := app.New(opts...)
	if err != nil {
		return nil, xerrors.Errorf("make base app, %w", err)
	}
	logger := baseApp.L()

	if !conf.Billingdb.Password.FromEnv("BILLINGDB_PASSWORD") {
		logger.Info("BILLINGDB_PASSWORD is empty")
	}

	if !conf.Kinesis.AccessKey.FromEnv("KINESIS_ACCESS_KEY") {
		logger.Info("KINESIS_ACCESS_KEY is empty")
	}

	if !conf.Kinesis.SecretKey.FromEnv("KINESIS_SECRET_KEY") {
		logger.Info("KINESIS_SECRET_KEY is empty")
	}

	bdb, err := bdbpg.New(conf.Billingdb, log.With(logger, log.String("cluster", bdbpg.DBName)))
	if err != nil {
		return nil, xerrors.Errorf("prepare BillingDB endpoint: %w", err)
	}

	a := &KinesisSenderApp{
		SenderApp: SenderApp{
			App: baseApp,
			BDB: bdb,
		},
		Config: conf,
	}

	kinesisWriter, err := kinesis.New(ctx, a.Config.Kinesis, a.L())
	if err != nil {
		return nil, xerrors.Errorf("LogBroker initialization failed: %w", err)
	}

	a.Sender = sender.New(btype, a.BDB, sender.FetchCommitAndSendBatch, kinesisWriter, a.L(), a.Config.Sender)
	if err = ready.Wait(ctx, a.Sender, &ready.DefaultErrorTester{Name: "databases", L: a.L()}, a.Config.InitTimeout.Duration); err != nil {
		return nil, xerrors.Errorf("wait backend: %w", err)
	}
	return a, err
}

func RunKinesisBillingMetricsSender(ctx context.Context, btype billingdb.BillType) int {
	a, err := NewKinesisMetricsSenderAppFromConfig(ctx, btype)
	if err != nil {
		fmt.Fprintf(os.Stderr, "sender failed to start app: %+v\n", err)
		return 1
	}

	a.L().Infof("Sender started to report billing metrics")
	defer a.App.Shutdown()

	if err := a.Sender.Serve(ctx); err != nil {
		a.L().Error("Failed to send billing metrics", log.Error(err))
		return 11
	}

	a.L().Infof("Sender has sent metrics successfully")
	return 0
}
