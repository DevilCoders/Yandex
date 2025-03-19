package app

import (
	"context"
	"fmt"
	"os"
	"os/signal"
	"syscall"

	"github.com/spf13/pflag"
	"golang.org/x/xerrors"

	inst_app "a.yandex-team.ru/cloud/mdb/internal/deprecated/app"
	lb_writer "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/logbroker"
	mdbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/producer"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type App struct {
	*inst_app.App
	Cfg    Config
	pr     *producer.Producer
	logger log.Logger
}

func NewAppFromConfig(ctx context.Context) (*App, error) {
	cfg := LoadConfig()

	logger, err := zap.New(zap.KVConfig(cfg.Config.LogLevel))
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize logger: %s", err.Error())
	}
	logger.Debugf("Using config %+v", cfg)

	cluster, err := pgutil.NewCluster(cfg.MetaDB, sqlutil.WithTracer(tracers.Log(logger)))
	if err != nil {
		return nil, xerrors.Errorf("cluster initialization failed: %w", err)
	}
	mdb := mdbpg.NewWithCluster(cluster, logger)

	lbWriter, err := lb_writer.New(ctx, cfg.LogBroker, lb_writer.Logger(logger))
	if err != nil {
		return nil, xerrors.Errorf("LogBroker initialization failed: %w", err)
	}

	pr, err := producer.New(logger, mdb, lbWriter, cfg.Producer)
	if err != nil {
		return nil, xerrors.Errorf("Producer initialization failed: %w", err)
	}
	instApp := inst_app.New(cfg.Config, logger)
	return NewAppCustom(logger, pr, cfg, instApp), nil
}

func NewAppCustom(logger log.Logger, pr *producer.Producer, cfg Config, instApp *inst_app.App) *App {
	app := &App{Cfg: cfg, pr: pr, logger: logger, App: instApp}
	return app
}

func (app *App) Run(ctx context.Context) error {
	app.logger.Info("the app is starting")
	err := app.pr.Run(ctx)
	if app.App != nil {
		app.App.Shutdown()
	}
	if err != nil && !xerrors.Is(err, context.Canceled) && !xerrors.Is(err, context.DeadlineExceeded) {
		app.logger.Error(err.Error())
		return err
	}
	app.logger.Info("the app is stopped")
	return nil
}

// ConfigureAndRun is a entry for cmd
func ConfigureAndRun() int {
	pflag.Parse()

	ctx := context.Background()
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	signals := make(chan os.Signal)
	signal.Notify(signals, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-signals
		cancel()
	}()

	app, err := NewAppFromConfig(ctx)
	if err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "failed to create app: %+v\n", err)
		return 2
	}
	err = app.Run(ctx)
	if err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "run error: %+v\n", err)
		return 1
	}
	return 0
}
