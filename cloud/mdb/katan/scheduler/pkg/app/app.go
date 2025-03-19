package app

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	holidayshttp "a.yandex-team.ru/cloud/mdb/internal/holidays/httpapi"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/weekends"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sentry/raven"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	kdbpg "a.yandex-team.ru/cloud/mdb/katan/internal/katandb/pg"
	"a.yandex-team.ru/cloud/mdb/katan/scheduler/internal/scheduler"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const ConfigPath = "/etc/yandex/mdb-katan/scheduler/scheduler.yaml"

type HolidaysConfig struct {
	Disabled bool                `json:"disabled" yaml:"disabled"`
	HTTP     holidayshttp.Config `json:"http" yaml:"http"`
}

type Config struct {
	LogLevel    log.Level             `json:"log_level" yaml:"log_level"`
	InitTimeout encodingutil.Duration `json:"init_timeout" yaml:"init_timeout"`
	Katandb     pgutil.Config         `json:"katandb" yaml:"katandb"`
	Scheduler   scheduler.Config      `json:"scheduler" yaml:"scheduler"`
	Sentry      raven.Config          `json:"sentry" yaml:"sentry"`
	Holidays    HolidaysConfig        `json:"holidays" yaml:"holidays"`
}

func DefaultConfig() Config {
	return Config{
		LogLevel:    log.InfoLevel,
		InitTimeout: encodingutil.FromDuration(time.Minute),
		Katandb: pgutil.Config{
			DB:   kdbpg.DBName,
			User: "katan",
		},
		Scheduler: scheduler.DefaultConfig(),
		Holidays: HolidaysConfig{
			HTTP: holidayshttp.DefaultConfig(),
		},
	}
}

type App struct {
	sher *scheduler.Scheduler
	cfg  Config
	L    log.Logger
}

func (app *App) waitForReady(ctx context.Context) error {
	awaitCtx, cancel := context.WithTimeout(ctx, app.cfg.InitTimeout.Duration)
	defer cancel()
	return ready.Wait(awaitCtx, app.sher, &ready.DefaultErrorTester{L: &nop.Logger{}}, time.Second)
}

func (app *App) Run(ctx context.Context) error {
	if err := app.sher.ProcessSchedules(ctx); err != nil {
		switch {
		case semerr.IsUnavailable(err):
			app.L.Error("process schedules failed cause katandb become unavailable", log.Error(err))
		default:
			sentry.GlobalClient().CaptureErrorAndWait(ctx, err, map[string]string{})
			app.L.Error("process schedules failed unexpectedly", log.Error(err))
		}
		return err
	}
	return nil
}

func (app *App) AddRollout(ctx context.Context, matchTags, commands, asUser string) (int64, error) {
	return app.sher.AddRollout(ctx, matchTags, commands, asUser, 10)
}

func (app *App) RolloutState(ctx context.Context, id int64) (scheduler.RolloutStateResponse, error) {
	return app.sher.RolloutState(ctx, id)
}

func NewAppCustom(ctx context.Context, config Config, kdb katandb.KatanDB, cal holidays.Calendar, L log.Logger) (*App, error) {
	sher := scheduler.New(kdb, cal, config.Scheduler, L)
	app := &App{
		sher: sher,
		cfg:  config,
		L:    L,
	}

	if err := app.waitForReady(ctx); err != nil {
		return nil, err
	}

	return app, nil
}

func newAppFromConfig(ctx context.Context, configPath string) (*App, error) {
	cfg := DefaultConfig()

	if err := config.LoadFromAbsolutePath(configPath, &cfg); err != nil {
		return nil, xerrors.Errorf("failed to load config: %w", err)
	}

	logger, err := zap.New(zap.KVConfig(cfg.LogLevel))
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize logger: %w", err)
	}
	logger.Debugf("config is %+v", cfg)

	if err = raven.Init(cfg.Sentry); err != nil {
		logger.Warn("running without sentry", log.Error(err))
	}
	var cal holidays.Calendar
	if cfg.Holidays.Disabled {
		cal = &weekends.Calendar{}
	} else {
		holidaysCal, err := holidayshttp.New(cfg.Holidays.HTTP, logger)
		if err != nil {
			return nil, err
		}
		cal = holidaysCal
	}

	kdb, err := kdbpg.New(cfg.Katandb, logger)
	if err != nil {
		return nil, err
	}

	app, err := NewAppCustom(ctx, cfg, kdb, cal, logger)
	if err != nil {
		_ = kdb.Close()
		return nil, err
	}

	return app, nil
}

func ProcessSchedules(configPath string) int {
	ctx := context.Background()
	app, err := newAppFromConfig(ctx, configPath)
	if err != nil {
		fmt.Println("scheduler initialization failed: ", err)
		return 2
	}

	if app.Run(ctx) != nil {
		return 1
	}
	return 0
}
