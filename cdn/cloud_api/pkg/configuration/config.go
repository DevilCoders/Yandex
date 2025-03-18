package configuration

import (
	"fmt"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type Config struct {
	LoggerConfig     LoggerConfig     `yaml:"logger_config"`
	ServerConfig     ServerConfig     `yaml:"server_config"`
	AuthClientConfig AuthClientConfig `yaml:"auth_client_config"`
	StorageConfig    StorageConfig    `yaml:"storage_config"`
	APIConfig        APIConfig        `yaml:"api_config"`
	DatabaseGCConfig DatabaseGCConfig `yaml:"database_gc_config"`
}

func (c *Config) Validate() error {
	err := valid.Struct(c,
		valid.Value(&c.LoggerConfig),
		valid.Value(&c.ServerConfig),
		valid.Value(&c.AuthClientConfig),
		valid.Value(&c.StorageConfig),
		valid.Value(&c.APIConfig),
		valid.Value(&c.DatabaseGCConfig),
	)
	if err != nil {
		return fmt.Errorf("%+v", err)
	}

	return nil
}

type LoggerConfig struct {
	Level    log.Level `yaml:"level"`
	Encoding string    `yaml:"encoding"`
	Path     string    `yaml:"path"`
}

type ServerConfig struct {
	ShutdownTimeout             time.Duration `yaml:"shutdown_timeout"`
	HTTPPort                    int           `yaml:"http_port"`
	GRPCPort                    int           `yaml:"grpc_port"`
	ConfigHandlerEnabled        bool          `yaml:"config_handler_enabled"`
	PprofEnabled                bool          `yaml:"pprof_enabled"`
	HTTPResponsePanicStacktrace bool          `yaml:"http_response_panic_stacktrace"`
	TLSConfig                   TLSConfig     `yaml:"tls_config"`
}

func (c *ServerConfig) Validate() error {
	return valid.Struct(c,
		valid.Value(&c.TLSConfig),
	)
}

type TLSConfig struct {
	UseTLS      bool   `yaml:"use_tls"`
	TLSCertPath string `yaml:"tls_cert_path"`
	TLSKeyPath  string `yaml:"tls_key_path"`
}

func (c *TLSConfig) Validate() error {
	var rules []valid.ValueRule

	if c.UseTLS {
		rules = append(rules, valid.Value(&c.TLSCertPath, rule.Required))
		rules = append(rules, valid.Value(&c.TLSKeyPath, rule.Required))
	}

	return valid.Struct(c, rules...)
}

type AuthClientConfig struct {
	EnableMock            bool          `yaml:"enable_mock"`
	Endpoint              string        `yaml:"endpoint"`
	UserAgent             string        `yaml:"user_agent"`
	ConnectionInitTimeout time.Duration `yaml:"connection_init_timeout"`
	MaxRetries            uint          `yaml:"max_retries"`
	RetryTimeout          time.Duration `yaml:"retry_timeout"`
	KeepAliveTime         time.Duration `yaml:"keep_alive_time"`
	KeepAliveTimeout      time.Duration `yaml:"keep_alive_timeout"`
}

func (a *AuthClientConfig) Validate() error {
	return valid.Struct(a,
		valid.Value(&a.Endpoint, rule.Required),
	)
}

type StorageConfig struct {
	DSN         string        `yaml:"dsn"`
	ReplicaDSN  string        `yaml:"replica_dsn"`
	MaxIdleTime time.Duration `yaml:"max_idle_time"`
	MaxLifeTime time.Duration `yaml:"max_life_time"`
	MaxIdleConn int           `yaml:"max_idle_conn"`
	MaxOpenConn int           `yaml:"max_open_conn"`
}

func (c *StorageConfig) Validate() error {
	return valid.Struct(c,
		valid.Value(&c.DSN, rule.Required),
	)
}

type APIConfig struct {
	FolderIDCacheSize int              `yaml:"folder_id_cache_size"`
	ConsoleAPIConfig  ConsoleAPIConfig `yaml:"console_api_config"`
	UserAPIConfig     UserAPIConfig    `yaml:"user_api_config"`
}

func (c *APIConfig) Validate() error {
	return valid.Struct(c,
		valid.Value(&c.FolderIDCacheSize, rule.GreaterOrEqual(100)),
	)
}

type ConsoleAPIConfig struct {
	EnableAutoActivateEntities bool `yaml:"enable_auto_activate_entities"`
}

type UserAPIConfig struct {
	EnableAutoActivateEntities bool `yaml:"enable_auto_activate_entities"`
}

type DatabaseGCConfig struct {
	EraseSoftDeleted EraseSoftDeleted `yaml:"erase_soft_deleted"`
	EraseOldVersions EraseOldVersions `yaml:"erase_old_versions"`
}

type EraseSoftDeleted struct {
	Enabled       bool          `yaml:"enabled"`
	TimeThreshold time.Duration `yaml:"time_threshold"`
}

type EraseOldVersions struct {
	Enabled          bool  `yaml:"enabled"`
	VersionThreshold int64 `yaml:"version_threshold"`
}

func (c *DatabaseGCConfig) Validate() error {
	return valid.Struct(c,
		valid.Value(&c.EraseSoftDeleted),
		valid.Value(&c.EraseOldVersions),
	)
}

func (c *EraseSoftDeleted) Validate() error {
	return valid.Struct(c,
		valid.Value(&c.TimeThreshold, rule.Required),
	)
}

func (c *EraseOldVersions) Validate() error {
	return valid.Struct(c,
		valid.Value(&c.VersionThreshold, rule.Required),
	)
}
