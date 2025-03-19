package monitoring

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const ConfigPath = "/etc/yandex/mdb-katan/monrun.yaml"

type Config struct {
	Katandb     pgutil.Config         `json:"katandb" yaml:"katandb"`
	InitTimeout encodingutil.Duration `json:"init_timeout" yaml:"init_timeout"`
}

func DefaultConfig() Config {
	return Config{
		Katandb:     pgutil.DefaultConfig(),
		InitTimeout: encodingutil.FromDuration(time.Second * 30),
	}
}

type Monrun struct {
	kdb katandb.KatanDB
	L   log.Logger
	cfg Config
}

func New(L log.Logger, kdb katandb.KatanDB, cfg Config) (*Monrun, error) {
	initCtx, cancel := context.WithTimeout(context.Background(), cfg.InitTimeout.Duration)
	defer cancel()

	if err := ready.Wait(initCtx, kdb, &ready.DefaultErrorTester{L: &nop.Logger{}}, time.Second); err != nil {
		return nil, xerrors.Errorf("Not ready with %s timeout: %s", cfg.InitTimeout.Duration, err)
	}
	return &Monrun{
		kdb: kdb, L: L, cfg: cfg,
	}, nil
}

func NewFromConfig(L log.Logger, configPath string) (*Monrun, error) {
	cfg := DefaultConfig()
	if err := config.Load(configPath, &cfg); err != nil {
		return nil, err
	}
	kdb, err := pg.New(cfg.Katandb, L)
	if err != nil {
		return nil, err
	}
	return New(L, kdb, cfg)
}
