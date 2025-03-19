package config

import "time"

type Monitoring struct {
	ListenEndpoint string `config:"listen-endpoint" yaml:"listen-endpoint"`

	PollInterval time.Duration `config:"poll_interval" yaml:"poll_interval"`
}
