package api

import "a.yandex-team.ru/cloud/mdb/internal/retry"

type Config struct {
	Retry            retry.Config `json:"retry" yaml:"retry"`
	ExposeErrorDebug bool         `json:"expose_error_debug" yaml:"expose_error_debug"`
}

func DefaultConfig() Config {
	cfg := Config{
		Retry: retry.DefaultConfig(),
	}

	// Hardcode default server-side retries in case they are changed in default package
	cfg.Retry.MaxRetries = 1
	return cfg
}
