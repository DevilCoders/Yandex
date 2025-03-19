package cfgtypes

import "a.yandex-team.ru/library/go/core/log"

type LoggingConf struct {
	Level          log.Level       `yaml:"level,omitempty" config:"level"`
	Paths          []string        `yaml:"paths,omitempty" config:"paths"`
	EnableJournald OverridableBool `yaml:"enable_journald,omitempty" config:"enable_journald"`
}

type TraceConf struct {
	Enabled            OverridableBool `yaml:"enabled,omitempty" config:"enabled"`
	QueueSize          int             `yaml:"queue_size,omitempty" config:"queue_size"`
	LocalAgentHostPort string          `yaml:"local_agent_hostport" config:"local_agent_hostport"`
}

type FeaturesConfig struct {
	Tz             string          `yaml:"tz,omitempty" config:"tz"`
	DropDuplicates OverridableBool `yaml:"drop_duplicates,omitempty" config:"drop_duplicates"`
}
