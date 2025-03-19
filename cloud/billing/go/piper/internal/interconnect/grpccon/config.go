package grpccon

import (
	"crypto/tls"
	"time"

	"github.com/imdario/mergo"
)

var defaultConfig = Config{
	RetryConfig: RetryConfig{
		MaxRetries:      2,
		PerRetryTimeout: 2 * time.Second,
	},
	KeepAliveConfig: KeepAliveConfig{
		Time:    10 * time.Second,
		Timeout: 1 * time.Second,
	},
}

type RetryConfig struct {
	MaxRetries      uint
	PerRetryTimeout time.Duration
}

type KeepAliveConfig struct {
	Time    time.Duration
	Timeout time.Duration
}

type Config struct {
	RetryConfig     RetryConfig
	KeepAliveConfig KeepAliveConfig
	TLS             *tls.Config
}

func extendConfigWithDefaults(config Config) Config {
	if err := mergo.Merge(&config, defaultConfig); err != nil {
		panic(err)
	}
	return config
}
