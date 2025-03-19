package app

import (
	"context"
	"fmt"
	"os"
	"os/signal"
	"syscall"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/logbroker"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	mdbpg "a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/producer"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type App struct {
	*app.App
	Cfg Config
	pr  *producer.Producer
}

func NewAppFromConfig(ctx context.Context) (*App, error) {
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(&cfg, ConfigName)...)
	if err != nil {
		return nil, err
	}

	lbWriter, err := logbroker.New(
		ctx, cfg.LogBroker, logbroker.Logger(baseApp.L()),
	)
	if err != nil {
		return nil, xerrors.Errorf("LogBroker initialization failed: %w", err)
	}

	cluster, err := pgutil.NewCluster(cfg.MetaDB, sqlutil.WithTracer(tracers.Log(baseApp.L())))
	if err != nil {
		return nil, xerrors.Errorf("cluster initialization failed: %w", err)
	}

	pr, err := producer.New(baseApp.L(), mdbpg.New(cluster, baseApp.L()), lbWriter, cfg.Producer)
	if err != nil {
		_ = cluster.Close()
		return nil, err
	}

	return NewAppCustom(pr, cfg, baseApp), nil
}

func NewAppCustom(pr *producer.Producer, cfg Config, baseApp *app.App) *App {
	return &App{Cfg: cfg, pr: pr, App: baseApp}
}

func (a *App) Run(ctx context.Context) error {
	L := a.App.L()
	L.Debug("the app is starting")

	err := a.pr.Run(ctx)
	if err != nil && !xerrors.Is(err, context.Canceled) && !xerrors.Is(err, context.DeadlineExceeded) {
		L.Error("unexpected error", log.Error(err))
		sentry.GlobalClient().CaptureErrorAndWait(ctx, err, nil)
		return err
	}
	L.Info("the app is stopped")
	return nil
}

// ConfigureAndRun is a entry for cmd
func ConfigureAndRun() int {
	ctx := context.Background()
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	signals := make(chan os.Signal)
	signal.Notify(signals, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-signals
		cancel()
	}()

	a, err := NewAppFromConfig(ctx)
	if err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "failed to create app: %+v\n", err)
		return 2
	}
	err = a.Run(ctx)
	if err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "run error: %+v\n", err)
		return 1
	}
	return 0
}
