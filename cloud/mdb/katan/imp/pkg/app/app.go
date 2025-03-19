package app

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	mdbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sentry/raven"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	implogic "a.yandex-team.ru/cloud/mdb/katan/imp/pkg/imp"
	kdbpg "a.yandex-team.ru/cloud/mdb/katan/internal/katandb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var ErrNotReady = xerrors.NewSentinel("not ready")

type Config struct {
	LogLevel    log.Level             `json:"log_level" yaml:"log_level"`
	Timeout     encodingutil.Duration `json:"timeout" yaml:"timeout"`
	InitTimeout encodingutil.Duration `json:"init_timeout" yaml:"init_timeout"`
	Metadb      pgutil.Config         `json:"metadb" yaml:"metadb"`
	Katandb     pgutil.Config         `json:"katandb" yaml:"katandb"`
	Sentry      raven.Config          `json:"sentry" yaml:"sentry"`
}

func DefaultConfig() Config {
	return Config{
		LogLevel:    log.InfoLevel,
		Timeout:     encodingutil.Duration{Duration: time.Minute * 5},
		InitTimeout: encodingutil.Duration{Duration: time.Second * 10},
		Metadb: pgutil.Config{
			User: "katan_imp",
			DB:   mdbpg.DBName,
		},
		Katandb: pgutil.Config{
			User: "katan_imp",
			DB:   kdbpg.DBName,
		},
	}
}

func waitForReady(ctx context.Context, imp *implogic.Imp, config Config) error {
	awaitCtx, cancel := context.WithTimeout(ctx, config.InitTimeout.Duration)
	defer cancel()
	return ready.Wait(awaitCtx, imp, &ready.DefaultErrorTester{L: &nop.Logger{}}, time.Second)
}

func RunWithContext(ctx context.Context, config Config, logger log.Logger) error {
	ctx, cancel := context.WithTimeout(ctx, config.Timeout.Duration)
	defer cancel()

	kdb, err := kdbpg.New(config.Katandb, log.With(logger, log.String("cluster", kdbpg.DBName)))
	if err != nil {
		return xerrors.Errorf("katanDB initialization failed: %w", err)
	}
	defer func() { _ = kdb.Close() }()

	mdb, err := mdbpg.New(config.Metadb, log.With(logger, log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return xerrors.Errorf("metaDB initialization failed: %w", err)
	}
	defer func() { _ = mdb.Close() }()

	imp := implogic.New(mdb, kdb, logger)

	if err = waitForReady(ctx, imp, config); err != nil {
		return ErrNotReady.Wrap(
			xerrors.Errorf("not ready within %s timeout: %s", config.InitTimeout.Duration, err))
	}

	return imp.FromMeta(ctx)
}

func Run(configPath string) int {
	cfg := DefaultConfig()
	if err := config.LoadFromAbsolutePath(configPath, &cfg); err != nil {
		fmt.Printf("failed to load config: %s\n", err)
		return 2
	}

	logger, err := zap.New(zap.KVConfig(cfg.LogLevel))
	if err != nil {
		fmt.Printf("failed to init logger: %s\n", err)
		return 2
	}
	logger.Debugf("config is %+v", cfg)

	if err = raven.Init(cfg.Sentry); err != nil {
		logger.Warn("running without sentry", log.Error(err))
	}

	err = RunWithContext(context.Background(), cfg, logger)

	if err != nil {
		switch {
		case xerrors.Is(err, ErrNotReady):
			logger.Error("initialization failed", log.Error(err))
			return 2
		case semerr.IsUnavailable(err):
			logger.Error("database not available", log.Error(err))
		case xerrors.Is(err, context.DeadlineExceeded):
			logger.Errorf("not succeed withing %s timeout: %s", cfg.Timeout, err)
		default:
			sentry.GlobalClient().CaptureErrorAndWait(context.Background(), err, map[string]string{})
			logger.Error("failed unexpectedly", log.Error(err))
		}
		return 1
	}
	return 0
}
