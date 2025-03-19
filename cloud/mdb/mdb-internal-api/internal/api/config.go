package api

import (
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
)

type Config struct {
	Retry             retry.Config                         `json:"retry" yaml:"retry"`
	ExposeErrorDebug  bool                                 `json:"expose_error_debug" yaml:"expose_error_debug"`
	CloudIDPrefix     string                               `json:"cloud_id_prefix" yaml:"cloud_id_prefix"`
	Domain            DomainConfig                         `json:"domain" yaml:"domain"`
	RestrictedChecker interceptors.RestrictedCheckerConfig `json:"restricted_checker" yaml:"restricted_checker"`
}

type DomainConfig struct {
	Public  string `json:"public" yaml:"public"`
	Private string `json:"private" yaml:"private"`
}

func DefaultConfig() Config {
	cfg := Config{
		Retry:         retry.DefaultConfig(),
		CloudIDPrefix: "mdb",
	}

	// Hardcode default server-side retries in case they are changed in default package
	cfg.Retry.MaxRetries = 1
	return cfg
}
