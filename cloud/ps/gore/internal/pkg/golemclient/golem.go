package golemclient

import (
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	Log       log.Logger
	BaseURL   string
	GolemGet  string
	GolemPost string
}

func DefaultConfig() *Config {
	return &Config{
		BaseURL:   "https://golem.yandex-team.ru/api/",
		GolemGet:  "get_object_resps.sbml",
		GolemPost: "set_object_resps.sbml",
	}
}
