package app

import (
	"a.yandex-team.ru/cloud/mdb/mdb-disklock/internal/disklock"
)

type Config struct {
	Disklock disklock.Config `json:"disklock" yaml:"disklock"`
}

func DefaultConfig() Config {
	return Config{
		Disklock: disklock.DefaultConfig(),
	}
}
