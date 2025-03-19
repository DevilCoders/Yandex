package config

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"time"

	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
	"github.com/imdario/mergo"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/loader"
	"a.yandex-team.ru/library/go/core/log"
)

type configContainer struct {
	files []string

	// backends init separated for tests
	configBackendsOnce initSync
	localLoader        *loader.Loader

	configLoadOnce initSync
	localConfig    LocalOnlyConfig
}

type LocalOnlyConfig struct {
	Log   cfgtypes.LoggingConf `yaml:"log" config:"log"`
	Trace cfgtypes.TraceConf   `yaml:"trace" config:"trace"`

	TLS     cfgtypes.TLSConfig     `yaml:"tls,omitempty" config:"tls"`
	YDB     cfgtypes.YDBConfig     `yaml:"ydb" config:"ydb"`
	IAMMeta cfgtypes.IAMMetaConfig `yaml:"iam_meta,omitempty" config:"iam_meta"`
	JWT     cfgtypes.JWTConfig     `yaml:"jwt,omitempty" config:"jwt"`
	TVM     cfgtypes.TVMConfig     `yaml:"tvm,omitempty" config:"tvm"`

	StatusServer cfgtypes.StatusServerConfig `yaml:"status_server,omitempty" config:"status_server"`

	Meta cfgtypes.PackageMeta `yaml:"meta,omitempty" config:"meta"`
}

func (c *Container) GetStatusServerConfig() (cfgtypes.StatusServerConfig, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.StatusServer, err
}

func (c *Container) GetLoggingConfig() (cfgtypes.LoggingConf, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.Log, err
}

func (c *Container) GetTracingConfig() (cfgtypes.TraceConf, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.Trace, err
}

func (c *Container) GetTLSConfig() (cfgtypes.TLSConfig, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.TLS, err
}

func (c *Container) GetYDBConfig() (cfgtypes.YDBConfig, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.YDB, err
}

func (c *Container) GetIAMMetaConfig() (cfgtypes.IAMMetaConfig, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.IAMMeta, err
}

func (c *Container) GetJWTConfig() (cfgtypes.JWTConfig, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.JWT, err
}

func (c *Container) GetTVMConfig() (cfgtypes.TVMConfig, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.TVM, err
}

func (c *Container) GetJWTKeyConfig(path string) (cfgtypes.JWTKey, error) {
	keyJSONFile, err := os.Open(path)
	defer func(keyJSONFile *os.File) {
		err := keyJSONFile.Close()
		if err != nil {
			c.logger.Error("failed to close jwt key file")
		}
	}(keyJSONFile)
	if err != nil {
		return cfgtypes.JWTKey{}, err
	}
	byteData, _ := ioutil.ReadAll(keyJSONFile)
	var key cfgtypes.JWTKey
	err = json.Unmarshal(byteData, &key)
	if err != nil {
		return cfgtypes.JWTKey{}, err
	}
	return key, nil
}

func (c *Container) GetConfigVersion() (string, error) {
	err := c.initializeLocalConfig()
	return c.localConfig.Meta.Version, err
}

func (c *Container) initializeLocalConfig() error {
	c.initializedLocalLoader()
	return c.configLoadOnce.Do(c.loadLocalConfig)
}

func (c *Container) initializedLocalLoader() {
	_ = c.configBackendsOnce.Do(c.makeLocalConfigLoader)
}

func (c *Container) makeLocalConfigLoader() error {
	if c.initFailed() {
		return c.initError
	}

	backends := []backend.Backend{}
	for _, fn := range c.files {
		backends = append(backends, loader.MergeFiles(fn))
	}
	backends = append(backends, env.NewBackend())
	c.localLoader = loader.New(backends...)
	return nil
}

var defaultLocalConfig = LocalOnlyConfig{
	Log: cfgtypes.LoggingConf{
		Level: log.InfoLevel,
		Paths: []string{"stderr"},
	},
	Trace: cfgtypes.TraceConf{
		QueueSize: 2048,
	},
	StatusServer: cfgtypes.StatusServerConfig{
		Port: 9741,
	},
	TLS: cfgtypes.TLSConfig{
		CAPath: "/etc/ssl/certs/ca-certificates.crt",
	},
	YDB: cfgtypes.YDBConfig{
		YDBParams: cfgtypes.YDBParams{
			ConnectTimeout:       cfgtypes.Seconds(time.Second * 5),
			MaxConnections:       64,
			MaxIdleConnections:   64,
			MaxDirectConnections: 8,
			ConnMaxLifetime:      cfgtypes.Seconds(time.Minute * 10),
		},
	},
}

func (c *Container) loadLocalConfig() error {
	_ = c.initializeLockboxBackend()
	if c.initFailed() {
		return c.initError
	}

	c.localConfig = defaultLocalConfig

	if err := c.localLoader.Load(c.mainCtx, &c.localConfig); err != nil {
		return c.failInitF("load local config: %w", err)
	}

	if c.lockboxLoader != nil {
		if err := c.lockboxLoader.Load(c.mainCtx, &c.localConfig); err != nil {
			return c.failInitF("load lockbox config: %w", err)
		}
	}

	if err := c.ydbInstallationDefault(&c.localConfig.YDB); err != nil {
		return c.failInitF("ydb installation set defaults error: %w", err)
	}
	return nil
}

func (c *Container) ydbInstallationDefault(cfg *cfgtypes.YDBConfig) error {
	if err := mergo.Merge(&cfg.Installations.Uniq, cfg.YDBParams); err != nil {
		return fmt.Errorf("uniq: %w", err)
	}
	if err := mergo.Merge(&cfg.Installations.Cumulative, cfg.YDBParams); err != nil {
		return fmt.Errorf("cumulative: %w", err)
	}
	return nil
}
