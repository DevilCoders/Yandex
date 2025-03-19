package config

import "time"

type MarketplaceClient struct {
	Endpoint string `config:"endpoint" yaml:"endpoint"`

	OpTimeout      time.Duration `config:"op-timeout" yaml:"op-timeout"`
	OpPollInterval time.Duration `config:"op-poll-interval" yaml:"op-poll-interval"`
}
