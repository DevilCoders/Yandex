package app

import (
	"a.yandex-team.ru/cloud/mdb/internal/app/environment"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sentry/raven"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/jaeger"
	"a.yandex-team.ru/library/go/core/log"
)

type SolomonConfig struct {
	Project    string `json:"project" yaml:"project"`
	Service    string `json:"service" yaml:"service"`
	Cluster    string `json:"cluster" yaml:"cluster"`
	URL        string `json:"url" yaml:"url"`
	OAuthToken string `json:"oauth_token,omitempty" yaml:"oauth_token,omitempty"`
	UseNameTag bool   `json:"use_name_tag,omitempty" yaml:"use_name_tag,omitempty"`
}

type PrometheusConfig struct {
	URL       string `json:"url" yaml:"url"`
	Namespace string `json:"namespace" yaml:"namespace"`
}

// Config is basic application config
type Config struct {
	Logging         LoggingConfig              `json:"logging" yaml:"logging"`
	Instrumentation httputil.ServeConfig       `json:"instrumentation,omitempty" yaml:"instrumentation,omitempty"`
	Sentry          raven.Config               `json:"sentry,omitempty" yaml:"sentry,omitempty"`
	Tracing         jaeger.Config              `json:"tracing" yaml:"tracing"`
	Solomon         SolomonConfig              `json:"solomon,omitempty" yaml:"solomon,omitempty"`
	Prometheus      PrometheusConfig           `json:"prometheus,omitempty" yaml:"prometheus,omitempty"`
	ServiceAccount  ServiceAccountConfig       `json:"service_account,omitempty" yaml:"service_account,omitempty"`
	Environment     environment.EnvironmentCfg `json:"environment,omitempty" yaml:"environment,omitempty"`
	AppName         string                     `json:"app_name,omitempty" yaml:"app_name,omitempty"`
}

type ServiceAccountConfig struct {
	ID         string        `json:"id" yaml:"id"`
	KeyID      secret.String `json:"key_id" yaml:"key_id"`
	PrivateKey secret.String `json:"private_key" yaml:"private_key"`
}

func (sac *ServiceAccountConfig) Empty() bool {
	return sac.PrivateKey.Unmask() == "" || sac.KeyID.Unmask() == "" || sac.ID == ""
}

func (sac *ServiceAccountConfig) FromEnv(prefix string) bool {
	return sac.PrivateKey.FromEnv(prefix + "_PRIVATE_KEY")
}

var _ AppConfig = &Config{}

func (c *Config) AppConfig() *Config {
	return c
}

// DefaultConfig returns default base config
func DefaultConfig() Config {
	return Config{
		Logging:         DefaultLoggingConfig(),
		Instrumentation: httputil.DefaultInstrumentationConfig(),
		Tracing:         jaeger.DefaultConfig(),
	}
}

func defaultConfigPtr() *Config {
	cfg := DefaultConfig()
	return &cfg
}

type LoggingConfig struct {
	Level log.Level `json:"level" yaml:"level"`
	File  string    `json:"file" yaml:"file"`
}

func DefaultLoggingConfig() LoggingConfig {
	return LoggingConfig{
		Level: log.DebugLevel,
	}
}

// AppConfig defines interface for a basic application config
type AppConfig interface {
	// AppConfig returns basic application config which must be of reference type - application might
	// change its values during setup
	AppConfig() *Config
}
