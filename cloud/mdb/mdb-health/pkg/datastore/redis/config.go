package redis

import (
	"os"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	// Name is backend's name
	Name = "redis"
)

const (
	configName = "mdbhdsredis.yaml"
	envAddrs   = "MDBH_DS_REDIS_ADDRS"
)

// Config represents backend configuration
type Config struct {
	Addrs       []string      `json:"addrs" yaml:"addrs"`
	MasterName  string        `json:"mastername" yaml:"mastername"`
	Username    string        `json:"username" yaml:"username"`
	Password    secret.String `json:"password" yaml:"password"`
	DB          int           `json:"db" yaml:"db"`
	LimitAggRec int           `json:"limitaggrec" yaml:"limitaggrec"`

	// Maximum number of retries before giving up.
	// Default is 3 retries; -1 (not 0) disables retries.
	MaxRetries int `json:"max_retries" yaml:"max_retries"`
	// Minimum backoff between each retry.
	// Default is 8 milliseconds; -1 disables backoff.
	MinRetryBackoff time.Duration `json:"min_retry_backoff" yaml:"min_retry_backoff"`
	// Maximum backoff between each retry.
	// Default is 512 milliseconds; -1 disables backoff.
	MaxRetryBackoff time.Duration `json:"max_retry_backoff" yaml:"max_retry_backoff"`

	// Dial timeout for establishing new connections.
	// Default is 5 seconds.
	DialTimeout time.Duration `json:"dial_timeout" yaml:"dial_timeout"`
	// Timeout for socket reads. If reached, commands will fail
	// with a timeout instead of blocking. Use value -1 for no timeout and 0 for default.
	// Default is 3 seconds.
	ReadTimeout time.Duration `json:"read_timeout" yaml:"read_timeout"`
	// Timeout for socket writes. If reached, commands will fail
	// with a timeout instead of blocking.
	// Default is ReadTimeout.
	WriteTimeout time.Duration `json:"write_timeout" yaml:"write_timeout"`

	// Type of connection pool.
	// true for FIFO pool, false for LIFO pool.
	// Note that fifo has higher overhead compared to lifo.
	PoolFIFO bool `json:"pool_fifo" yaml:"pool_fifo"`
	// Maximum number of socket connections.
	// Default is 10 connections per every available CPU as reported by runtime.GOMAXPROCS.
	PoolSize int `json:"pool_size" yaml:"pool_size"`
	// Minimum number of idle connections which is useful when establishing
	// new connection is slow.
	MinIdleConns int `json:"min_idle_conns" yaml:"min_idle_conns"`
	// Connection age at which client retires (closes) the connection.
	// Default is to not close aged connections.
	MaxConnAge time.Duration `json:"max_conn_age" yaml:"max_conn_age"`
	// Amount of time client waits for connection if all connections
	// are busy before returning an error.
	// Default is ReadTimeout + 1 second.
	PoolTimeout time.Duration `json:"pool_timeout" yaml:"pool_timeout"`
	// Amount of time after which client closes idle connections.
	// Should be less than server's timeout.
	// Default is 5 minutes. -1 disables idle timeout check.
	IdleTimeout time.Duration `json:"idle_timeout" yaml:"idle_timeout"`
	// Frequency of idle checks made by idle connections reaper.
	// Default is 1 minute. -1 disables idle connections reaper,
	// but idle connections are still discarded by the client
	// if IdleTimeout is set.
	IdleCheckFrequency time.Duration `json:"idle_check_frequency" yaml:"idle_check_frequency"`
}

func DefaultConfig() Config {
	return Config{
		Addrs:       []string{"localhost:6379"},
		DB:          1,
		LimitAggRec: 1000,
	}
}

func init() {
	datastore.RegisterBackend(Name, NewWithConfigLoad)
}

// LoadConfig loads Redis datastore configuration
func LoadConfig(logger log.Logger) (Config, error) {
	cfg := DefaultConfig()
	if err := config.Load(configName, &cfg); err != nil {
		logger.Errorf("failed to load redis datastore config, using defaults: %s", err)
		cfg = DefaultConfig()
	}

	if addrs, ok := os.LookupEnv(envAddrs); ok {
		cfg.Addrs = strings.Split(addrs, ",")
	}

	if !cfg.Password.FromEnv("REDIS_AUTH_TOKEN") {
		logger.Info("REDIS_AUTH_TOKEN is empty")
	}

	return cfg, nil
}

// NewWithConfigLoad loads configuration for Redis datastore and constructs it
func NewWithConfigLoad(logger log.Logger) (datastore.Backend, error) {
	cfg, err := LoadConfig(logger)
	if err != nil {
		return nil, err
	}

	return New(logger, cfg), nil
}
