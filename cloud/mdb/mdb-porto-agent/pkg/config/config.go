package config

import (
	"context"
	"os"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config is a simple porto-agent config (no secrets here for now)
type Config struct {
	LogLevel       log.Level `yaml:"log_level"`
	StoreLibPath   string    `yaml:"store_lib_path"`
	StoreCachePath string    `yaml:"store_cache_path"`
	DbmURL         string    `yaml:"dbm_url"`
}

// DefaultConfig is a default config constructor
func DefaultConfig() Config {
	return Config{
		LogLevel:       log.InfoLevel,
		StoreLibPath:   "/var/lib/porto_agent_states",
		StoreCachePath: "/var/cache/porto_agent_states",
		DbmURL:         "https://mdb.yandex-team.ru",
	}
}

// LoadConfig loads config from path
func LoadConfig(path string, logger log.Logger) (Config, error) {
	expanded, err := tilde.Expand(path)
	if err != nil {
		return Config{}, xerrors.Errorf("failed to expand config path: %w", err)
	}

	config := DefaultConfig()
	if _, err := os.Stat(expanded); os.IsNotExist(err) {
		logger.Warnf("path %q is empty, using default config", expanded)
	} else {
		logger.Debugf("loading config from %q", expanded)
		loader := confita.NewLoader(file.NewBackend(expanded))

		if err = loader.Load(context.Background(), &config); err != nil {
			return Config{}, xerrors.Errorf("failed to parse config: %w", err)
		}
	}

	logger.Debugf("loaded config: %+v", config)
	return config, nil
}

// FromEnv returns Config stored in env
func FromEnv(env *cli.Env) Config {
	return *env.Config.(*Config)
}
