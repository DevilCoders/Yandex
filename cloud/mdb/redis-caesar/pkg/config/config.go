package config

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config/flags"
)

//nolint: gochecknoglobals
var defaultConfig = Global{
	DCSWaitTimeout: time.Second,
	PingInterval:   time.Second,
	PingTimeout:    10 * time.Second,
	Redis: &RedisConfig{
		ClusterName: "mymaster",
		Nodes: map[internal.RedisHost]RedisCreds{
			"127.0.0.1:6379": {},
		},
	},
	ZooKeeper: &ZKConfig{
		SessionTimeout: 2 * time.Second,
		Namespace:      "",
		Hosts:          []string{"127.0.0.1:2181"},
	},
	Telemetry: &TelemetryConfig{
		Address:   "localhost:8000",
		Profiling: true,
	},
}

// Global is a struct that stores global config.
type Global struct {
	SelfHostname   string           `config:"hostname"`
	DCSWaitTimeout time.Duration    `config:"dcs_wait_timeout"`
	PingInterval   time.Duration    `config:"ping_interval"`
	PingTimeout    time.Duration    `config:"ping_interval"`
	Redis          *RedisConfig     `config:"redis,required"`
	ZooKeeper      *ZKConfig        `config:"zookeeper,required"`
	Telemetry      *TelemetryConfig `config:"telemetry"`
}

// RedisConfig is a struct that keeps configuration of redis hosts.
type RedisConfig struct {
	ClusterName string                            `config:"cluster_name,required"`
	Nodes       map[internal.RedisHost]RedisCreds `config:"nodes,required"`
}

type RedisCreds struct {
	User     string `config:"user,required"`
	Password string `config:"password,required"`
}

// ZKConfig is struct that stores ZooKeeper connection config.
type ZKConfig struct {
	SessionTimeout time.Duration `config:"session_timeout"`
	Namespace      string        `config:"namespace,required"`
	Hosts          []string      `config:"hosts,required"`
	SelfHostname   string
}

type TelemetryConfig struct {
	LogLevel  string `config:"log_level"`
	Address   string `config:"address"`
	Profiling bool   `config:"profiling"`
}

// Load is a function that takes path to config file and loads it.
func Load(ctx context.Context, flagsData flags.Root) (*Global, error) {
	config := defaultConfig
	if flagsData.ConfigFile != "" {
		loader := confita.NewLoader(file.NewBackend(flagsData.ConfigFile))
		if err := loader.Load(ctx, &config); err != nil {
			return nil, fmt.Errorf("failed to load config from %s: %w", flagsData.ConfigFile, err)
		}
	}
	config = mapRootFlagsToConfig(flagsData, config)
	config, err := addDynamicValues(config)
	if err != nil {
		return nil, fmt.Errorf("unable to map dynamic values to config: %w", err)
	}

	return &config, nil
}

func mapRootFlagsToConfig(flagsData flags.Root, config Global) Global {
	config.Telemetry.LogLevel = flagsData.LogLevel

	return config
}

func addDynamicValues(config Global) (Global, error) {
	if config.SelfHostname != "" {
		hostname, err := os.Hostname()
		if err != nil {
			return config, fmt.Errorf("unable to get hostname for config: %w", err)
		}
		config.SelfHostname = hostname
	}
	config.ZooKeeper.SelfHostname = config.SelfHostname

	return config, nil
}
