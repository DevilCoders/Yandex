package config

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"

	"gopkg.in/yaml.v2"
)

const ConfigFileName = "mssync.yaml"

type Config struct {
	App                      app.Config `yaml:"app"`
	ConnectionString         string     `yaml:"connection_string"`
	HealthCheckInterval      int        `yaml:"health_check_interval_s"`
	CooldownInterval         int        `yaml:"cooldown_interval_s"`
	Hostname                 string     `yaml:"hostname"`
	MaxErrCount              int        `yaml:"max_error_count"`
	LowerPromotionThreshold  int        `yaml:"lower_promotion_threshold"`
	HigherPromotionThreshold int        `yaml:"higher_promotion_threshold"`
	FailureConditionLevel    int        `yaml:"failure_condition_level"`
	LockFile                 string     `yaml:"lock_file"`
	PromotionTimeout         int        `yaml:"promotion_timeout_s"`
}

func (cfg *Config) Validate() error {
	return nil
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func ReadConfig(configFile string) (*Config, error) {
	var cfg Config
	f, err := os.Open(configFile)
	if err != nil {
		err = fmt.Errorf("failed to open config file %s: %s", configFile, err.Error())
		return nil, err
	}
	defer f.Close()

	decoder := yaml.NewDecoder(f)
	err = decoder.Decode(&cfg)
	if err != nil {
		err = fmt.Errorf("failed to load config from %s: %s", configFile, err.Error())
		return nil, err
	}
	return &cfg, nil
}

func DefaultConfig() Config {
	return Config{
		App:                      app.DefaultConfig(),
		ConnectionString:         "server=localhost;user id=;",
		HealthCheckInterval:      5,
		CooldownInterval:         100,
		MaxErrCount:              100,
		LowerPromotionThreshold:  3600,
		HigherPromotionThreshold: 1800,
		FailureConditionLevel:    3,
		PromotionTimeout:         180,
	}
}

func ConfigureLogger(cfg app.LoggingConfig) (log.Logger, error) {
	zapConfig := zap.ConsoleConfig(cfg.Level)
	if cfg.File != "" {
		zapConfig.OutputPaths = []string{cfg.File}
	}
	return zap.New(zapConfig)
}
