package config

import (
	"context"
	"fmt"
	"time"

	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
	"github.com/imdario/mergo"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/ydb"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/loader"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
)

type configOverrideContainer struct {
	overrideConfig bool

	dbConfiguratorOnce initSync
	dbConfigurator     NamespaceConfigurator

	piperOverrideBackendsOnce initSync
	piperOverrideLoader       *loader.Loader

	piperOverrideOnce initSync
	piperConfig       PiperOverriddenConfig
}

type ApplicationConfig struct {
	LocalOnlyConfig       `yaml:",inline"`
	PiperOverriddenConfig `yaml:",inline"`
}

func (c *Container) GetApplicationConfig() (ApplicationConfig, error) {
	_ = c.initializeLocalConfig()
	err := c.initializePiperConfigOverride()

	return ApplicationConfig{
		LocalOnlyConfig:       c.localConfig,
		PiperOverriddenConfig: c.piperConfig,
	}, err
}

// PiperOverriddenConfig is part of config with override from ydb context in namespace "piper"
type PiperOverriddenConfig struct {
	ResourceManager        cfgtypes.RMConfig                         `yaml:"resource_manager" config:"resource_manager"`
	TeamIntegration        cfgtypes.TeamIntegrationConfig            `yaml:"team_integration" config:"team_integration"`
	UnifiedAgent           cfgtypes.UAConfig                         `yaml:"unified_agent" config:"unified_agent"`
	LogbrokerInstallations map[string]*cfgtypes.LbInstallationConfig `yaml:"logbroker_installations" config:"logbroker_installations"`
	Clickhouse             cfgtypes.Clickhouse                       `yaml:"clickhouse" config:"clickhouse"`

	Resharder cfgtypes.ResharderConfig `yaml:"resharder" config:"resharder"`
	Dump      cfgtypes.DumperConfig    `yaml:"dump" config:"dump"`
	Features  cfgtypes.FeaturesConfig  `yaml:"features,omitempty" config:"features"`
}

const piperNamespace = "piper"

func (c *Container) GetRMConfig() (cfgtypes.RMConfig, error) {
	err := c.initializePiperConfigOverride()
	return c.piperConfig.ResourceManager, err
}

func (c *Container) GetTIConfig() (cfgtypes.TeamIntegrationConfig, error) {
	err := c.initializePiperConfigOverride()
	return c.piperConfig.TeamIntegration, err
}

func (c *Container) GetUAConfig() (cfgtypes.UAConfig, error) {
	err := c.initializePiperConfigOverride()
	return c.piperConfig.UnifiedAgent, err
}

func (c *Container) GetLbInstallationConfig(name string) (cfgtypes.LbInstallationConfig, error) {
	err := c.initializePiperConfigOverride()
	cfg := c.piperConfig.LogbrokerInstallations[name]
	if cfg == nil {
		return cfgtypes.LbInstallationConfig{}, c.failInitF("no such logbroker installation '%s'", name)
	}
	return *cfg, err
}

func (c *Container) GetClickhouseConfig() (cfgtypes.Clickhouse, error) {
	err := c.initializePiperConfigOverride()
	return c.piperConfig.Clickhouse, err
}

func (c *Container) GetResharderConfig() (cfgtypes.ResharderConfig, error) {
	err := c.initializePiperConfigOverride()
	return c.piperConfig.Resharder, err
}

func (c *Container) GetDumperConfig() (cfgtypes.DumperConfig, error) {
	err := c.initializePiperConfigOverride()
	return c.piperConfig.Dump, err
}

func (c *Container) GetFeaturesConfig() (cfgtypes.FeaturesConfig, error) {
	err := c.initializePiperConfigOverride()
	return c.piperConfig.Features, err
}

func (c *Container) initializePiperConfigOverride() error {
	_ = c.piperOverrideBackendsOnce.Do(c.makePiperOverrideLoader)
	return c.piperOverrideOnce.Do(c.loadPiperConfig)
}

func (c *Container) makePiperOverrideLoader() error {
	c.initializedOverrideConfigurator()
	if c.initFailed() {
		return c.initError
	}

	c.debug("make config overrides backends")
	overrides, err := c.dbConfigurator.GetConfig(c.mainCtx, piperNamespace)
	if err != nil {
		return c.failInitF("load config overrides for namespace %s: %w", piperNamespace, err)
	}

	backend := backend.Func(piperNamespace, func(_ context.Context, key string) ([]byte, error) {
		if v, ok := overrides[key]; ok {
			return []byte(v), nil
		}
		return nil, backend.ErrNotFound
	})

	// env is most significant source so set it before overrides
	c.piperOverrideLoader = loader.New(backend, env.NewBackend())
	return nil
}

func (c *Container) initializedOverrideConfigurator() {
	_ = c.dbConfiguratorOnce.Do(c.makeOverrideConfigurator)
}

func (c *Container) makeOverrideConfigurator() error {
	if c.initFailed() {
		return c.initError
	}
	if !c.overrideConfig {
		c.dbConfigurator = c.dummy()
		return nil
	}

	db, err := c.GetYDB()
	if err != nil {
		return err
	}
	cfg, err := c.GetYDBConfig()
	if err != nil {
		return err
	}

	c.dbConfigurator = ydb.NewConfigurator(c.mainCtx, db, cfg.Root)
	return nil
}

var defaultPiperConfig = PiperOverriddenConfig{
	Clickhouse: cfgtypes.Clickhouse{
		ClickhouseCommonConfig: cfgtypes.ClickhouseCommonConfig{
			Port:               9000,
			MaxConnections:     4,
			MaxIdleConnections: 4,
			ConnMaxLifetime:    cfgtypes.Seconds(time.Minute * 10),
		},
	},

	ResourceManager: cfgtypes.RMConfig{
		Auth: cfgtypes.AuthConfig{
			Type: cfgtypes.IAMMetaAuthType,
		},
	},
	TeamIntegration: cfgtypes.TeamIntegrationConfig{
		Auth: cfgtypes.AuthConfig{
			Type: cfgtypes.JWTAuthType,
		},
	},
	Resharder: cfgtypes.ResharderConfig{
		Sink: cfgtypes.ResharderSinkConfig{
			Logbroker: cfgtypes.LogbrokerSinkConfig{
				Enabled:     cfgtypes.BoolTrue,
				Route:       "source_metrics",
				MaxParallel: 32,
				SplitSize:   cfgtypes.DataSize(50 * cfgtypes.MiB),
			},
			LogbrokerErrors: cfgtypes.LogbrokerSinkConfig{
				Enabled:     cfgtypes.BoolTrue,
				Route:       "invalid_metrics",
				MaxParallel: 32,
				SplitSize:   cfgtypes.DataSize(50 * cfgtypes.MiB),
			},
			YDBErrors: cfgtypes.YDBSinkConfig{Enabled: cfgtypes.BoolTrue},
		},
		MetricsGrace: cfgtypes.Seconds(time.Hour * 9),
	},
	Dump: cfgtypes.DumperConfig{
		Sink: cfgtypes.DumpSinkConfig{
			YDBErrors:        cfgtypes.YDBSinkConfig{Enabled: cfgtypes.BoolTrue},
			ClickhouseErrors: cfgtypes.ClickhouseSinkConfig{Enabled: cfgtypes.BoolTrue},
		},
	},
}

func (c *Container) loadPiperConfig() error {
	_ = c.initializeLockboxBackend()
	_ = c.initializeLocalConfig()
	if c.initFailed() {
		return c.initError
	}

	c.debug("loading config with overrides")
	c.piperConfig = defaultPiperConfig

	if err := c.localLoader.Load(c.mainCtx, &c.piperConfig); err != nil {
		return c.failInitF("load piper config: %w", err)
	}
	c.debug("config loaded from local sources")

	if c.lockboxLoader != nil {
		if err := c.lockboxLoader.Load(c.mainCtx, &c.piperConfig); err != nil {
			return c.failInitF("load piper lockbox config: %w", err)
		}
		c.debug("config loaded from lockbox")
	}

	if err := c.piperOverrideLoader.Load(c.mainCtx, &c.piperConfig); err != nil {
		return c.failInitF("load piper overrides: %w", err)
	}
	c.debug("config loaded from overrides")

	if err := c.resharderSourceDefault(c.piperConfig.Resharder.Source); err != nil {
		return err
	}
	c.debug("resharder configs loaded")

	if err := c.dumpSourceDefault(c.piperConfig.Dump.Source); err != nil {
		return err
	}
	c.debug("dump configs loaded")

	if err := c.initializeFeatures(); err != nil {
		return err
	}

	return nil
}

var defaultResharderSource = cfgtypes.ResharderSourceConfig{
	Logbroker: cfgtypes.LogbrokerSourceConfig{
		MaxMessages:  1000,
		MaxSize:      cfgtypes.MiB, // This is compressed data size
		Lag:          12 * 24 * cfgtypes.Seconds(time.Hour),
		BatchLimit:   5000,
		BatchSize:    2 * cfgtypes.MiB,
		BatchTimeout: 15 * cfgtypes.Seconds(time.Second),
	},
	Parallel: 1,
	Params: cfgtypes.ResharderHandlerConfig{
		ChunkSize:      50 * cfgtypes.MiB,
		MetricLifetime: 12 * 24 * cfgtypes.Seconds(time.Hour),
		MetricGrace:    9 * cfgtypes.Seconds(time.Hour),
	},
}

func (c *Container) resharderSourceDefault(sources map[string]*cfgtypes.ResharderSourceConfig) error {
	for name, cfg := range sources {
		if err := mergo.Merge(cfg, defaultResharderSource); err != nil {
			return fmt.Errorf("resharding source %s set defaults error: %w", name, err)
		}
	}
	return nil
}

var defaultDumpSource = cfgtypes.DumpSourceConfig{
	Logbroker: cfgtypes.LogbrokerSourceConfig{
		MaxMessages:  1000,
		MaxSize:      cfgtypes.MiB, // This is compressed data size
		Lag:          12 * 24 * cfgtypes.Seconds(time.Hour),
		BatchLimit:   5000,
		BatchSize:    2 * cfgtypes.MiB,
		BatchTimeout: 15 * cfgtypes.Seconds(time.Second),
	},
}

func (c *Container) dumpSourceDefault(sources map[string]*cfgtypes.DumpSourceConfig) error {
	for name, cfg := range sources {
		if err := mergo.Merge(cfg, defaultDumpSource); err != nil {
			return fmt.Errorf("dump source %s set defaults error: %w", name, err)
		}
	}
	return nil
}

func (c *Container) initializeFeatures() error {
	config := c.piperConfig.Features

	flags := features.Default()

	if config.Tz != "" {
		tz, err := timetool.ParseTz(config.Tz)
		if err != nil {
			return err
		}
		flags.Set(features.LocalTimezone(tz))
	}

	if config.DropDuplicates.Bool() {
		flags.Set(features.DropDuplicates(config.DropDuplicates.Bool()))
	}

	features.SetDefault(flags)
	return nil
}
