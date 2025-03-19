package reindexer

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/reindexer/logic"
)

const configName = "mdb-search-reindexer.yaml"

type Config struct {
	app.Config
	APIConfigPath      string                `json:"api_config_path" yaml:"api_config_path"`
	MaxSequentialFails int                   `json:"max_sequential_fails" yaml:"max_sequential_fails"`
	Timeout            encodingutil.Duration `json:"timeout" yaml:"timeout"`
}

func DefaultConfig() Config {
	return Config{
		MaxSequentialFails: 42,
		Config:             app.DefaultConfig(),
		APIConfigPath:      "/etc/yandex/mdb-internal-api/mdb-internal-api.yaml",
		Timeout:            encodingutil.FromDuration(time.Hour * 4),
	}
}

func Run() int {
	cfg := DefaultConfig()
	baseApp, err := app.New(append(app.DefaultToolOptions(&cfg, configName), app.WithSentry())...)
	if err != nil {
		fmt.Printf("failed to create base App: %s\n", err)
		return 1
	}
	intAPIConfig, err := cli.LoadConfig(cfg.APIConfigPath, baseApp.L())
	if err != nil {
		baseApp.L().Errorf("failed to load mdb-internal-api config: %s", err)
		return 1
	}
	reindexer, err := logic.New(intAPIConfig, baseApp.L(), cfg.MaxSequentialFails)
	if err != nil {
		baseApp.L().Errorf("failed to create reindexer: %s", err)
		return 1
	}
	ctx, cancel := context.WithTimeout(context.Background(), cfg.Timeout.Duration)
	defer cancel()
	if err := reindexer.Run(ctx); err != nil {
		baseApp.L().Errorf("reindex failed: %s", err)
		return 2
	}
	return 0
}
