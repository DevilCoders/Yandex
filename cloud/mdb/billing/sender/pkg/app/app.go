package app

import (
	"time"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	bdbpg "a.yandex-team.ru/cloud/mdb/billing/internal/billingdb/pg"
	"a.yandex-team.ru/cloud/mdb/billing/sender/internal/sender"
	writerMocks "a.yandex-team.ru/cloud/mdb/billing/sender/internal/writer/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	AppName = "sender"
)

type Config struct {
	App         app.Config            `json:"app" yaml:"app"`
	Sender      sender.Config         `json:"sender" yaml:"sender"`
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
			User: "billing_sender",
			DB:   bdbpg.DBName,
		},
		Sender:      sender.DefaultConfig(),
		InitTimeout: encodingutil.Duration{Duration: time.Second * 10},
	}
}

type SenderApp struct {
	*app.App
	BDB    billingdb.BillingDB
	Sender *sender.Sender
}

func NewMockMetricsSenderFromExtDeps(btype billingdb.BillType, bdb billingdb.BillingDB, logger log.Logger, cfg sender.Config, writeSucceed bool) (*sender.Sender, error) {
	ctl := gomock.NewController(logger)
	writerMock := writerMocks.NewMockWriter(ctl)
	writerMock.EXPECT().Write(gomock.Any(), gomock.Any()).DoAndReturn(func(docs []writer.Doc, timeout time.Duration) error {
		if writeSucceed {
			return nil
		}
		return xerrors.Errorf("write failed")
	}).AnyTimes()

	return sender.New(btype, bdb, sender.FetchAndSendBatch, writerMock, logger, cfg), nil
}
