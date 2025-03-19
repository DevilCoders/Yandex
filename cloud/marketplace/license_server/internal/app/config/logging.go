package config

import "a.yandex-team.ru/library/go/core/log"

type Logger struct {
	Level    log.Level `config:"level" yaml:"level"`
	FilePath []string  `config:"file-paths" yaml:"file-paths"`

	EnableJournald bool `config:"enable-journald" yaml:"enable-journald"`
}
